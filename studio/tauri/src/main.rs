// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::process::Command;

fn main() {
  let output = Command::new("sh")
          .arg("-c")
          .arg("$HOME/shogstudio/bin/studio run")
          .spawn()
          .expect("failed to execute process");

   
  tauri::Builder::default()
    .run(tauri::generate_context!())
    .expect("error while running tauri application");
}
