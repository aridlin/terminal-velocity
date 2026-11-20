//go:build linux

package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/http/cookiejar"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"strings"
	"syscall"
	"time"
	"unsafe"
)

// Set at build time for isolated API integration tests.
var accountEndpoint = "https://aridlin.pl/terminal-velocity/api.php"
var gameProgram = "/game/terminal_velocity"

type accountResponse struct {
	Username string  `json:"username"`
	CSRF     string  `json:"csrf"`
	Save     *string `json:"save"`
	Revision int     `json:"revision"`
	Error    string  `json:"error"`
}

type accountAPI struct {
	endpoint string
	client   *http.Client
	csrf     string
}

func newAPI(endpoint string) *accountAPI {
	jar, _ := cookiejar.New(nil)
	return &accountAPI{
		endpoint: endpoint,
		client: &http.Client{
			Jar: jar, Timeout: 8 * time.Second,
			CheckRedirect: func(*http.Request, []*http.Request) error {
				return http.ErrUseLastResponse
			},
		},
	}
}

func (a *accountAPI) call(method, action string, payload any) (accountResponse, error) {
	var result accountResponse
	var body io.Reader
	if payload != nil {
		data, err := json.Marshal(payload)
		if err != nil {
			return result, err
		}
		body = bytes.NewReader(data)
	}
	req, err := http.NewRequest(method, a.endpoint+"?action="+action, body)
	if err != nil {
		return result, err
	}
	if action == "save" {
		req.Header.Set("X-TV-Save-Format", "portable")
	}
	if method != "GET" {
		req.Header.Set("Content-Type", "application/json")
		req.Header.Set("X-TV-CSRF", a.csrf)
	}
	resp, err := a.client.Do(req)
	if err != nil {
		return result, errors.New("unavailable")
	}
	defer resp.Body.Close()
	if json.NewDecoder(io.LimitReader(resp.Body, 1200000)).Decode(&result) != nil {
		return result, errors.New("unavailable")
	}
	if resp.StatusCode != http.StatusOK {
		if result.Error != "" {
			return result, errors.New(result.Error)
		}
		return result, errors.New("unavailable")
	}
	if result.CSRF != "" {
		a.csrf = result.CSRF
	}
	return result, nil
}

type accountMenu struct {
	api            *accountAPI
	reader         *bufio.Reader
	guest, profile string
	user, synced   string
	revision       int
	conflict       bool
	notice         string
}

func (m *accountMenu) ask(label string) (string, error) {
	fmt.Print(label)
	line, err := m.reader.ReadString('\n')
	return strings.TrimRight(line, "\r\n"), err
}

func (m *accountMenu) password() (string, error) {
	fd := os.Stdin.Fd()
	var current syscall.Termios
	_, _, code := syscall.Syscall(syscall.SYS_IOCTL, fd, syscall.TCGETS, uintptr(unsafe.Pointer(&current)))
	if code != 0 {
		return "", code
	}
	hidden := current
	hidden.Lflag &^= syscall.ECHO
	_, _, code = syscall.Syscall(syscall.SYS_IOCTL, fd, syscall.TCSETS, uintptr(unsafe.Pointer(&hidden)))
	if code != 0 {
		return "", code
	}
	defer syscall.Syscall(syscall.SYS_IOCTL, fd, syscall.TCSETS, uintptr(unsafe.Pointer(&current)))
	password, err := m.ask("Password: ")
	fmt.Println()
	return password, err
}

func safeMessage(err error) string {
	switch err.Error() {
	case "invalid_credentials":
		return "Incorrect username or password."
	case "credentials_format":
		return "Use a 3–24 character username and a password of at least 8 characters."
	case "name_unavailable":
		return "That username is taken."
	case "rate_limit":
		return "Too many attempts. Try again later."
	case "session_expired", "csrf", "login_required":
		return "The account session expired. Sign in again."
	case "save_conflict":
		return "Another device has a newer save. Choose which save to keep."
	case "too_large":
		return "This save exceeds the account limit. The visit's save is still available."
	case "save_format":
		return "The account rejected this save format. Your local save is still available."
	default:
		return "Account sync is unavailable. You can keep playing locally."
	}
}

func remoteSave(result accountResponse) string {
	if result.Save == nil {
		return ""
	}
	return *result.Save
}

func (m *accountMenu) attach(action string) error {
	name, err := m.ask("Username: ")
	if err != nil {
		return err
	}
	password, err := m.password()
	if err != nil {
		return err
	}
	if _, err := m.api.call("GET", "session", nil); err != nil {
		return err
	}
	identity, err := m.api.call("POST", action, map[string]string{"username": name, "password": password})
	if err != nil {
		return err
	}
	remote, err := m.api.call("GET", "save", nil)
	if err != nil {
		_, _ = m.api.call("POST", "logout", struct{}{})
		return err
	}
	save := remoteSave(remote)
	working := save
	if save == "" {
		guest, err := readSave(m.guest)
		if err != nil && !os.IsNotExist(err) {
			_, _ = m.api.call("POST", "logout", struct{}{})
			return err
		}
		if guest != "" {
			choice, err := m.ask("Use this visit's guest progress for the new account save? [y/N] ")
			if err != nil {
				return err
			}
			if strings.EqualFold(choice, "y") {
				working = guest
			}
		}
	}
	folder, err := os.MkdirTemp(filepath.Dir(m.guest), "account.")
	if err != nil {
		_, _ = m.api.call("POST", "logout", struct{}{})
		return err
	}
	m.profile = filepath.Join(folder, "campaign")
	if err := m.replaceSave(working); err != nil {
		_, _ = m.api.call("POST", "logout", struct{}{})
		return err
	}
	m.user, m.revision, m.synced, m.conflict = identity.Username, remote.Revision, save, false
	return m.sync()
}

func saveTool(profile string, arguments ...string) *exec.Cmd {
	command := exec.Command(gameProgram, arguments...)
	command.Env = append(os.Environ(), "TV_ICEBERG_FILE="+profile)
	return command
}

func readSave(profile string) (string, error) {
	primary, err := os.ReadFile(profile)
	if err != nil {
		return "", err
	}
	for _, suffix := range []string{".routes", ".routes.bak"} {
		if _, err := os.Stat(profile + suffix); os.IsNotExist(err) {
			continue
		} else if err != nil {
			return "", err
		}
		backup, err := saveTool(profile, "--account-save-export").Output()
		return string(backup), err
	}
	return string(primary), nil
}

func restorePortableSave(profile, save string) error {
	file, err := os.CreateTemp(filepath.Dir(profile), "import.*")
	if err != nil {
		return err
	}
	defer os.Remove(file.Name())
	_, writeErr := file.WriteString(save)
	closeErr := file.Close()
	if writeErr != nil {
		return writeErr
	}
	if closeErr != nil {
		return closeErr
	}
	return saveTool(profile, "--account-save-import", file.Name()).Run()
}

func (m *accountMenu) replaceSave(save string) error {
	if strings.HasPrefix(save, "TVBACKUP1 ") {
		return restorePortableSave(m.profile, save)
	}
	if save == "" {
		err := os.Remove(m.profile)
		if err != nil && !os.IsNotExist(err) {
			return err
		}
	} else {
		next := m.profile + ".next"
		if err := os.WriteFile(next, []byte(save), 0600); err != nil {
			return err
		}
		if err := os.Rename(next, m.profile); err != nil {
			return err
		}
	}
	// The game may try an older fallback if the primary is unavailable.
	if err := os.Remove(m.profile + ".bak"); err != nil && !os.IsNotExist(err) {
		return err
	}
	return nil
}

func (m *accountMenu) sync() error {
	if m.user == "" {
		return nil
	}
	if m.conflict {
		return errors.New("save_conflict")
	}
	save, err := readSave(m.profile)
	if os.IsNotExist(err) {
		return nil
	}
	if err != nil {
		return err
	}
	if save == m.synced {
		return nil
	}
	result, err := m.api.call("PUT", "save", map[string]any{"revision": m.revision, "save": save})
	if err != nil {
		m.conflict = err.Error() == "save_conflict"
		return err
	}
	m.revision, m.synced = result.Revision, save
	return nil
}

func (m *accountMenu) refresh() error {
	remote, err := m.api.call("GET", "save", nil)
	if err != nil {
		return err
	}
	if remote.Revision != m.revision {
		local, err := readSave(m.profile)
		if err != nil && !os.IsNotExist(err) {
			return err
		}
		if local != m.synced {
			m.conflict = true
			return errors.New("save_conflict")
		}
		if err := m.replaceSave(remoteSave(remote)); err != nil {
			return err
		}
		m.revision, m.synced = remote.Revision, remoteSave(remote)
	}
	return m.sync()
}

func (m *accountMenu) resolve() error {
	remote, err := m.api.call("GET", "save", nil)
	if err != nil {
		return err
	}
	choice, err := m.ask("1  Continue with account save\n2  Keep this SSH save\n3  Decide later\n> ")
	if err != nil {
		return err
	}
	switch choice {
	case "1":
		if err := m.replaceSave(remoteSave(remote)); err != nil {
			return err
		}
		m.revision, m.synced, m.conflict = remote.Revision, remoteSave(remote), false
	case "2":
		// Upload against the revision whose choice was shown, so a third
		// device writing during the prompt cannot be overwritten silently.
		m.revision, m.synced, m.conflict = remote.Revision, remoteSave(remote), false
		return m.sync()
	}
	return nil
}

func (m *accountMenu) play() bool {
	profile := m.guest
	if m.user != "" {
		if !m.conflict {
			if err := m.refresh(); err != nil {
				m.notice = safeMessage(err)
				if m.conflict {
					return true
				}
			}
		}
		profile = m.profile
	}
	game := exec.Command(gameProgram, "--campaign", "--ssh", "--mute")
	game.Stdin, game.Stdout, game.Stderr = os.Stdin, os.Stdout, os.Stderr
	game.Env = append(os.Environ(), "TV_ICEBERG_FILE="+profile, "TV_PROGRESS_FILE="+profile+".records")
	if err := game.Start(); err != nil {
		m.notice = "Could not open the game."
		return true
	}
	wait := make(chan error, 1)
	go func() { wait <- game.Wait() }()
	ticker := time.NewTicker(18 * time.Second)
	defer ticker.Stop()
	interrupt := make(chan os.Signal, 1)
	signal.Notify(interrupt, syscall.SIGHUP, syscall.SIGTERM)
	defer signal.Stop(interrupt)
	for {
		select {
		case <-wait:
			m.finishSync()
			return true
		case <-ticker.C:
			m.finishSync()
		case sig := <-interrupt:
			_ = game.Process.Signal(sig)
			select {
			case <-wait:
			case <-time.After(2 * time.Second):
				_ = game.Process.Kill()
				<-wait
			}
			m.finishSync()
			return false
		}
	}
}

func (m *accountMenu) finishSync() {
	if err := m.sync(); err != nil {
		m.notice = safeMessage(err)
	}
}

func (m *accountMenu) menu() {
	for {
		fmt.Println("\nTERMINAL VELOCITY")
		if m.notice != "" {
			fmt.Println(m.notice)
			m.notice = ""
		}
		if m.user == "" {
			fmt.Println("1  Play as guest\n2  Sign in\n3  Create account\nQ  Exit")
		} else {
			syncLabel := "Sync save"
			if m.conflict {
				syncLabel = "Resolve save conflict"
			}
			fmt.Printf("Account  %s\n1  Play\n2  %s\n3  Sign out\nQ  Exit\n", m.user, syncLabel)
		}
		choice, err := m.ask("> ")
		if err != nil {
			return
		}
		switch strings.ToLower(strings.TrimSpace(choice)) {
		case "1":
			if !m.play() {
				return
			}
		case "2":
			if m.user == "" {
				err = m.attach("login")
			} else if m.conflict {
				err = m.resolve()
			} else {
				err = m.refresh()
				if err == nil {
					m.notice = "Account save is up to date."
				}
			}
		case "3":
			if m.user == "" {
				err = m.attach("register")
			} else if m.conflict {
				err = errors.New("save_conflict")
			} else if err = m.sync(); err == nil {
				_, err = m.api.call("POST", "logout", struct{}{})
				if err == nil {
					m.user = ""
				}
			}
		case "q":
			if err := m.sync(); err != nil {
				fmt.Println(safeMessage(err))
				answer, err := m.ask("The SSH save is still unsynced. Exit this visit anyway? [y/N] ")
				if err != nil || !strings.EqualFold(answer, "y") {
					continue
				}
			}
			if m.user != "" {
				_, _ = m.api.call("POST", "logout", struct{}{})
			}
			return
		}
		if err != nil {
			m.notice = safeMessage(err)
		}
	}
}

func main() {
	guest := os.Getenv("TV_ICEBERG_FILE")
	if guest == "" {
		fmt.Fprintln(os.Stderr, "Game session directory is unavailable.")
		os.Exit(1)
	}
	menu := accountMenu{api: newAPI(accountEndpoint), reader: bufio.NewReader(os.Stdin), guest: guest}
	menu.menu()
}
