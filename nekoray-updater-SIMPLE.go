package main

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
)

const Version = "v1.0.3"

func main() {
	fmt.Printf("NekoRay Updater %s\n", Version)

	// Get executable info
	exe, err := os.Executable()
	if err != nil {
		log.Fatal("Failed to get executable path:", err)
	}

	wd := filepath.Dir(exe)
	os.Chdir(wd)
	exeName := filepath.Base(os.Args[0])
	
	fmt.Printf("Executable: %s\n", exeName)
	fmt.Printf("Working directory: %s\n", wd)

	// Check if this is an updater call
	if strings.Contains(strings.ToLower(exeName), "updater") {
		fmt.Println("Running updater functionality...")
		handleUpdate()
		return
	}

	// Check if this is a launcher call
	if strings.Contains(strings.ToLower(exeName), "launcher") {
		fmt.Println("Running launcher functionality...")
		handleLaunch()
		return
	}

	fmt.Println("Usage:")
	fmt.Println("  updater    - Update NekoRay components")
	fmt.Println("  launcher   - Launch NekoRay daemon/CLI")
}

func handleUpdate() {
	fmt.Println("=== NekoRay Update Process ===")
	
	// Check for components to update
	components := []string{"nekoray-cli", "nekoray-web", "nekoray-daemon", "nekobox_core"}
	
	for _, comp := range components {
		if fileExists(comp + ".new") {
			fmt.Printf("Updating %s...\n", comp)
			if copyFile(comp+".new", comp) {
				os.Remove(comp + ".new")
				fmt.Printf("✅ Updated %s\n", comp)
			} else {
				fmt.Printf("❌ Failed to update %s\n", comp)
			}
		}
	}
	
	fmt.Println("Update process completed")
	
	// Start the main application
	startMainApp()
}

func handleLaunch() {
	fmt.Println("=== NekoRay Launcher ===")
	
	// Check which components are available and start appropriate one
	if fileExists("nekoray-daemon") {
		fmt.Println("Starting NekoRay Daemon...")
		cmd := exec.Command("./nekoray-daemon", "--port", "8080")
		cmd.Start()
	} else if fileExists("nekoray-cli") {
		fmt.Println("Starting NekoRay CLI...")
		cmd := exec.Command("./nekoray-cli", "daemon")
		cmd.Start()
	} else {
		fmt.Println("No NekoRay components found to launch")
	}
}

func startMainApp() {
	var cmd *exec.Cmd
	
	if runtime.GOOS == "windows" {
		if fileExists("nekoray.exe") {
			cmd = exec.Command("./nekoray.exe")
		} else if fileExists("nekobox.exe") {
			cmd = exec.Command("./nekobox.exe")
		}
	} else {
		// Linux/Unix
		if fileExists("nekoray-daemon") {
			cmd = exec.Command("./nekoray-daemon")
		} else if fileExists("nekoray") {
			cmd = exec.Command("./nekoray")
		} else if fileExists("nekobox") {
			cmd = exec.Command("./nekobox")
		}
	}
	
	if cmd != nil {
		fmt.Println("Starting main application...")
		cmd.Start()
	} else {
		fmt.Println("No main application found to start")
	}
}

func fileExists(filename string) bool {
	_, err := os.Stat(filename)
	return err == nil
}

func copyFile(src, dst string) bool {
	data, err := os.ReadFile(src)
	if err != nil {
		return false
	}
	
	err = os.WriteFile(dst, data, 0755)
	return err == nil
}