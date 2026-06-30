tracehub_runner_resolve_executable() {
    local root_dir="$1"
    local env_tracehub_bin="${TRACEHUB_BIN:-}"
    local candidate

    if [ -n "$env_tracehub_bin" ]; then
        if [ -f "$env_tracehub_bin" ] && [ -x "$env_tracehub_bin" ]; then
            printf "%s\n" "$env_tracehub_bin"
            return 0
        fi

        printf "tracehub executable configured by TRACEHUB_BIN is not executable: %s\n" "$env_tracehub_bin" >&2
        return 1
    fi

    for candidate in \
        "$root_dir/tracehub" \
        "$root_dir/install/bin/tracehub"
    do
        if [ -f "$candidate" ] && [ -x "$candidate" ]; then
            printf "%s\n" "$candidate"
            return 0
        fi
    done

    printf "tracehub executable not found\n" >&2
    printf "Set TRACEHUB_BIN or provide one of:\n" >&2
    printf "  %s\n" "$root_dir/tracehub" >&2
    printf "  %s\n" "$root_dir/install/bin/tracehub" >&2
    return 1
}

tracehub_runner_validate_module_options() {
    if [ "$FORCE_RELOAD" -eq 1 ] && [ "$USE_LOADED_MODULE" -eq 1 ]; then
        print_error "--force-reload and --use-loaded-module are mutually exclusive"
        return 1
    fi

    return 0
}

tracehub_runner_require_module_update_privilege() {
    if [ "$(id -u)" -ne 0 ]; then
        print_error "Loading or replacing SharedMem requires root privileges"
        print_info "Use --use-loaded-module only when SharedMem is already loaded and $DEVICE_PATH is readable and writable"
        return 1
    fi

    return 0
}

tracehub_runner_require_device_access() {
    local device_path="$1"

    if [ ! -e "$device_path" ]; then
        print_error "Device node $device_path does not exist"
        return 1
    fi

    if [ ! -r "$device_path" ] || [ ! -w "$device_path" ]; then
        print_error "$device_path is not readable and writable"
        return 1
    fi

    return 0
}

tracehub_runner_load_kernel_module() {
    local ko_path="${KO_PATH:-/lib/modules/$(uname -r)/SharedMem.ko}"
    local module_rtt_region_addr_dec
    local mem_size_dec

    tracehub_runner_validate_module_options || return 1

    if lsmod | grep -q "^SharedMem"; then
        if [ "$FORCE_RELOAD" -eq 1 ]; then
            tracehub_runner_require_module_update_privilege || return 1

            print_info "Unloading existing SharedMem module..."
            if ! rmmod SharedMem; then
                print_error "Failed to unload existing SharedMem module"
                return 1
            fi
            sleep 1
        elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
            tracehub_runner_require_device_access "$DEVICE_PATH" || return 1
            print_success "Using already loaded SharedMem module"
            return 0
        else
            print_error "SharedMem module is already loaded"
            print_info "Use --use-loaded-module to reuse it or --force-reload to replace it"
            return 1
        fi
    elif [ "$USE_LOADED_MODULE" -eq 1 ]; then
        print_error "--use-loaded-module requires an already loaded SharedMem module"
        return 1
    fi

    tracehub_runner_require_module_update_privilege || return 1

    if [ ! -f "$ko_path" ]; then
        print_error "Kernel module not found: $ko_path"
        print_info "Please install the kernel module or specify the path with -k/--ko option"
        return 1
    fi

    module_rtt_region_addr_dec=$((RTT_REGION_ADDR))
    mem_size_dec=$((MEM_SIZE))

    print_info "Loading SharedMem module (rtt_region_addr=$RTT_REGION_ADDR, rtt_region_size=$MEM_SIZE)..."
    print_info "Kernel module: $ko_path"
    if ! insmod "$ko_path" phys_addrs="$module_rtt_region_addr_dec" mem_sizes="$mem_size_dec"; then
        print_error "Failed to load kernel module"
        return 1
    fi
    MODULE_LOADED_BY_SCRIPT=1

    sleep 1
    if ! tracehub_runner_require_device_access "$DEVICE_PATH"; then
        if [ "$MODULE_LOADED_BY_SCRIPT" -eq 1 ]; then
            rmmod SharedMem 2>/dev/null || true
            MODULE_LOADED_BY_SCRIPT=0
        fi
        return 1
    fi

    print_success "Kernel module loaded successfully"
    return 0
}
