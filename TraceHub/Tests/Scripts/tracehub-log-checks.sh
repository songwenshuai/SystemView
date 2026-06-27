contains_c1_control_bytes() {
    local log_file="$1"

    od -An -tx1 -v "$log_file" | awk '
        function hex_value(hex,    i, c, n, p) {
            n = 0
            for (i = 1; i <= length(hex); i++) {
                c = tolower(substr(hex, i, 1))
                p = index("0123456789abcdef", c)
                if (p == 0) {
                    return -1
                }
                n = n * 16 + p - 1
            }
            return n
        }
        function clear_utf8() {
            utf8_need = 0
            utf8_len = 0
            utf8_code = 0
            utf8_min = 0
        }
        function start_utf8(byte) {
            if (byte >= 194 && byte <= 223) {
                utf8_need = 1
                utf8_len = 2
                utf8_code = byte % 32
                utf8_min = 128
                return 1
            }
            if (byte >= 224 && byte <= 239) {
                utf8_need = 2
                utf8_len = 3
                utf8_code = byte % 16
                utf8_min = 2048
                return 1
            }
            if (byte >= 240 && byte <= 244) {
                utf8_need = 3
                utf8_len = 4
                utf8_code = byte % 8
                utf8_min = 65536
                return 1
            }
            return 0
        }
        function finish_utf8() {
            if (utf8_code < utf8_min ||
                (utf8_code >= 55296 && utf8_code <= 57343) ||
                utf8_code > 1114111 ||
                (utf8_code >= 128 && utf8_code <= 159)) {
                found = 1
                clear_utf8()
                return 0
            }
            clear_utf8()
            return 1
        }
        {
            for (i = 1; i <= NF; i++) {
                byte = hex_value($i)
                reprocess = 1
                while (reprocess) {
                    reprocess = 0
                    if (utf8_need > 0) {
                        if (byte >= 128 && byte <= 191) {
                            utf8_code = utf8_code * 64 + (byte % 64)
                            utf8_need--
                            if (utf8_need == 0) {
                                finish_utf8()
                            }
                            continue
                        }
                        clear_utf8()
                        reprocess = 1
                        continue
                    }
                    if (byte >= 128 && byte <= 159) {
                        found = 1
                        continue
                    }
                    if (byte >= 160 && byte <= 191) {
                        continue
                    }
                    start_utf8(byte)
                }
            }
        }
        END {
            if (found) {
                exit 0
            }
            exit 1
        }
    '
}
