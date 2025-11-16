package main

import (
	"fmt"
	"os"

	"github.com/spf13/cobra"
	boxmain "github.com/sagernet/sing-box/cmd/sing-box"
	"github.com/sagernet/sing-box/constant"
)

const Version_neko = "v1.0.3"

var rootCmd = &cobra.Command{
	Use:   "nekobox_core",
	Short: "NekoBox Core - sing-box based proxy core for NekoRay",
	Long:  "A fast, powerful proxy core based on sing-box, designed for NekoRay CLI/Web interfaces",
	Run:   runMain,
}

func runMain(cmd *cobra.Command, args []string) {
	fmt.Println("sing-box:", constant.Version, "NekoBox:", Version_neko)
	fmt.Println()

	// If called with nekobox argument, enter nekobox_core mode
	if len(os.Args) > 1 && os.Args[1] == "nekobox" {
		fmt.Println("NekoBox Core mode - ready for gRPC communication")
		fmt.Println("Note: gRPC server implementation not included in standalone version")
		fmt.Println("Use sing-box mode for direct operation")
		return
	}

	// Default: run as sing-box
	boxmain.Main()
}

func init() {
	rootCmd.Flags().BoolVarP(&versionFlag, "version", "v", false, "Show version information")
	rootCmd.Flags().BoolVarP(&helpFlag, "help", "h", false, "Show help")
}

var versionFlag bool
var helpFlag bool

func main() {
	if len(os.Args) > 1 {
		if os.Args[1] == "--version" || os.Args[1] == "-v" {
			fmt.Printf("nekobox_core %s (sing-box %s)\n", Version_neko, constant.Version)
			return
		}
		if os.Args[1] == "--help" || os.Args[1] == "-h" {
			rootCmd.Help()
			return
		}
	}

	if err := rootCmd.Execute(); err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		os.Exit(1)
	}
}