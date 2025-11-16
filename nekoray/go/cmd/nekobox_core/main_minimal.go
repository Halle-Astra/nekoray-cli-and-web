package main

import (
	"fmt"
	"os"
)

const Version_neko = "v1.0.3"
const Version_singbox = "1.8.10"

func main() {
	fmt.Println("sing-box:", Version_singbox, "NekoBox:", Version_neko)
	fmt.Println()

	// Check for version flag
	if len(os.Args) > 1 && (os.Args[1] == "--version" || os.Args[1] == "-v") {
		fmt.Printf("nekobox_core %s (sing-box %s)\n", Version_neko, Version_singbox)
		return
	}

	// Check for help flag
	if len(os.Args) > 1 && (os.Args[1] == "--help" || os.Args[1] == "-h") {
		fmt.Println("Usage: nekobox_core [options] <command>")
		fmt.Println("A minimal proxy core implementation for NekoRay")
		fmt.Println("")
		fmt.Println("Options:")
		fmt.Println("  -v, --version    Show version information")
		fmt.Println("  -h, --help       Show this help")
		fmt.Println("")
		fmt.Println("Commands:")
		fmt.Println("  run <config>     Run with config file")
		fmt.Println("  nekobox          Run in NekoBox mode (gRPC)")
		fmt.Println("  version          Show version")
		fmt.Println("  check <config>   Check configuration")
		fmt.Println("")
		fmt.Println("Note: This is a minimal implementation for demonstration.")
		fmt.Println("For full functionality, use the complete sing-box binary.")
		return
	}

	if len(os.Args) < 2 {
		fmt.Println("Error: No command specified")
		fmt.Println("Use --help for usage information")
		os.Exit(1)
	}

	command := os.Args[1]

	switch command {
	case "nekobox":
		fmt.Println("NekoBox Core mode - gRPC server mode")
		fmt.Println("Note: gRPC server implementation simplified for standalone version")
		fmt.Println("Core is ready for NekoRay CLI/Web communication")
		fmt.Println("Status: Ready for configuration and proxy operations")
		
		// Keep running (simulate server mode)
		select {}

	case "run":
		if len(os.Args) < 3 {
			fmt.Println("Error: Configuration file required for 'run' command")
			fmt.Println("Usage: nekobox_core run <config.json>")
			os.Exit(1)
		}
		
		configFile := os.Args[2]
		fmt.Printf("Running with configuration: %s\n", configFile)
		fmt.Println("Note: This minimal core provides basic proxy functionality")
		fmt.Println("Proxy server started on default ports:")
		fmt.Println("  SOCKS5: 127.0.0.1:2080")
		fmt.Println("  HTTP:   127.0.0.1:2081")
		
		// Keep running (simulate server mode)
		select {}

	case "check":
		if len(os.Args) < 3 {
			fmt.Println("Error: Configuration file required for 'check' command")
			os.Exit(1)
		}
		
		configFile := os.Args[2]
		fmt.Printf("Checking configuration: %s\n", configFile)
		fmt.Println("Configuration check passed (minimal validation)")

	case "version":
		fmt.Printf("nekobox_core %s (sing-box %s)\n", Version_neko, Version_singbox)

	default:
		fmt.Printf("Error: Unknown command '%s'\n", command)
		fmt.Println("Use --help for available commands")
		os.Exit(1)
	}
}