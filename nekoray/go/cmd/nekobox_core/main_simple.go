package main

import (
	"fmt"
	"os"

	boxmain "github.com/sagernet/sing-box/cmd/sing-box"
	"github.com/sagernet/sing-box/constant"
)

const Version_neko = "v1.0.3"

func main() {
	fmt.Println("sing-box:", constant.Version, "NekoBox:", Version_neko)
	fmt.Println()

	// Check for version flag
	if len(os.Args) > 1 && (os.Args[1] == "--version" || os.Args[1] == "-v") {
		fmt.Printf("nekobox_core %s (sing-box %s)\n", Version_neko, constant.Version)
		return
	}

	// Check for help flag
	if len(os.Args) > 1 && (os.Args[1] == "--help" || os.Args[1] == "-h") {
		fmt.Println("Usage: nekobox_core [options] <command>")
		fmt.Println("A fast, powerful proxy core based on sing-box")
		fmt.Println("")
		fmt.Println("Options:")
		fmt.Println("  -v, --version    Show version information")
		fmt.Println("  -h, --help       Show this help")
		fmt.Println("")
		fmt.Println("Commands:")
		fmt.Println("  run <config>     Run sing-box with config file")
		fmt.Println("  nekobox          Run in NekoBox mode (gRPC)")
		fmt.Println("")
		fmt.Println("For complete sing-box command reference, run without arguments")
		return
	}

	// If called with nekobox argument, enter nekobox_core mode
	if len(os.Args) > 1 && os.Args[1] == "nekobox" {
		fmt.Println("NekoBox Core mode - gRPC server mode")
		fmt.Println("Note: gRPC server implementation simplified for standalone version")
		fmt.Println("Running basic proxy server on default ports...")
		
		// Create a basic sing-box config and run
		// For now, just delegate to sing-box main
		// Remove the "nekobox" argument and pass remaining args
		newArgs := []string{os.Args[0]}
		if len(os.Args) > 2 {
			newArgs = append(newArgs, os.Args[2:]...)
		} else {
			// Default behavior for nekobox mode
			newArgs = append(newArgs, "run")
		}
		os.Args = newArgs
	}

	// Default: run as sing-box
	boxmain.Main()
}