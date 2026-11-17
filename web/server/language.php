<?php
// Polish IP ranges extracted from DB-IP IP to Country Lite, November 2026.
// CC BY 4.0, https://db-ip.com/db/download/ip-to-country-lite
declare(strict_types=1);

function terminal_velocity_language(string $address, string $path): ?string {
    if (str_starts_with(strtolower($address), '::ffff:')) {
        $mapped = inet_pton($address);
        if ($mapped !== false) {
            $address = inet_ntop(substr($mapped, 12));
        }
    }
    if (!filter_var($address, FILTER_VALIDATE_IP, FILTER_FLAG_NO_PRIV_RANGE | FILTER_FLAG_NO_RES_RANGE)) {
        return null;
    }
    $address = inet_pton($address);
    $ranges = @file_get_contents($path);
    if ($ranges === false || strlen($ranges) < 4) {
        return null;
    }
    $v4Count = unpack('Ncount', $ranges)['count'];
    $v6Start = 4 + $v4Count * 8;
    $v6Bytes = strlen($ranges) - $v6Start;
    if ($v6Bytes < 0 || $v6Bytes % 32 !== 0) {
        return null;
    }
    $width = strlen($address);
    $start = $width === 4 ? 4 : $v6Start;
    $low = 0;
    $high = ($width === 4 ? $v4Count : intdiv($v6Bytes, 32)) - 1;
    while ($low <= $high) {
        $middle = intdiv($low + $high, 2);
        $offset = $start + $middle * $width * 2;
        if (strcmp($address, substr($ranges, $offset, $width)) < 0) {
            $high = $middle - 1;
        } elseif (strcmp($address, substr($ranges, $offset + $width, $width)) > 0) {
            $low = $middle + 1;
        } else {
            return 'pl';
        }
    }
    return 'en';
}

header('Content-Type: application/json');
header('Cache-Control: private, no-store');
echo json_encode(['language' => terminal_velocity_language($_SERVER['REMOTE_ADDR'] ?? '', __DIR__ . '/pl-address-ranges.bin')]);
