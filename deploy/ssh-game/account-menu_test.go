//go:build linux

package main

import (
	"bufio"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestSharedAccountSave(t *testing.T) {
	const csrf = "test-session-csrf"
	remote, revision := "browser save", 7
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		reply := func(v any) { _ = json.NewEncoder(w).Encode(v) }
		switch r.URL.Query().Get("action") {
		case "session":
			http.SetCookie(w, &http.Cookie{Name: "test", Value: "token", Path: "/"})
			reply(map[string]string{"csrf": csrf})
		case "login":
			_, cookieError := r.Cookie("test")
			if cookieError != nil || r.Header.Get("X-TV-CSRF") != csrf {
				http.Error(w, "session missing", 403)
				return
			}
			reply(accountResponse{Username: "guest-test", CSRF: csrf})
		case "save":
			if r.Method == "GET" {
				reply(accountResponse{Save: &remote, Revision: revision})
				return
			}
			var input struct {
				Revision int    `json:"revision"`
				Save     string `json:"save"`
			}
			if json.NewDecoder(r.Body).Decode(&input) != nil || r.Header.Get("X-TV-CSRF") != csrf {
				http.Error(w, "bad request", 400)
				return
			}
			if input.Revision != revision {
				w.WriteHeader(409)
				reply(accountResponse{Error: "save_conflict"})
				return
			}
			remote, revision = input.Save, revision+1
			reply(accountResponse{Revision: revision})
		}
	}))
	defer server.Close()

	api := newAPI(server.URL)
	if _, err := api.call("GET", "session", nil); err != nil {
		t.Fatal(err)
	}
	if _, err := api.call("POST", "login", map[string]string{"username": "guest-test", "password": "fixture-only"}); err != nil {
		t.Fatal(err)
	}
	menu := accountMenu{api: api, user: "guest-test", profile: filepath.Join(t.TempDir(), "campaign")}
	if err := menu.refresh(); err != nil {
		t.Fatal(err)
	}
	local, _ := os.ReadFile(menu.profile)
	if string(local) != "browser save" {
		t.Fatal("SSH did not receive the browser's save")
	}
	if err := os.WriteFile(menu.profile, []byte("ssh progress"), 0600); err != nil {
		t.Fatal(err)
	}
	if err := menu.sync(); err != nil || remote != "ssh progress" || revision != 8 {
		t.Fatalf("SSH did not publish progress: %v", err)
	}
	remote, revision = "newer from browser", 9
	if err := os.WriteFile(menu.profile, []byte("different SSH progress"), 0600); err != nil {
		t.Fatal(err)
	}
	if err := menu.sync(); err == nil || !menu.conflict || remote != "newer from browser" {
		t.Fatal("an intervening browser write was not protected")
	}
	menu.reader = bufio.NewReader(strings.NewReader("1\n"))
	if err := menu.resolve(); err != nil {
		t.Fatal(err)
	}
	local, _ = os.ReadFile(menu.profile)
	if string(local) != remote || menu.conflict {
		t.Fatal("account conflict choice did not apply the latest save")
	}
}
