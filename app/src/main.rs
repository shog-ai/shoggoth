// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use shog_studio::handle_session;
use shog_studio::log;
use shog_studio::sleep_ms;
use shog_studio::Args;
use shog_studio::LogType::*;
use shog_studio::SHOG_HELP;
use shog_studio::SHOG_VERSION;

#[cfg(not(target_os = "windows"))]
fn redirect_stdout(runtime_path: &str) {
    use std::fs::File;
    use std::os::unix::io::AsRawFd;

    let logs_path = format!("{runtime_path}/logs.txt");

    // Open a file to redirect output to
    let file = File::create(&logs_path).unwrap();

    // Redirect stdout
    unsafe {
        libc::dup2(file.as_raw_fd(), libc::STDOUT_FILENO);
    }

    // Redirect stderr
    let file_clone = file.try_clone().unwrap();
    unsafe {
        libc::dup2(file_clone.as_raw_fd(), libc::STDERR_FILENO);
    }
}

#[cfg(target_os = "windows")]
fn redirect_stdout(_runtime_path: &str) {}

fn start_backend() {
    let args: Vec<String> = std::env::args().collect();

    let mut parsed_args: Args = match Args::parse(args) {
        Ok(res) => res,

        Err(err) => {
            log(ERROR, &err);
            std::process::exit(1);
        }
    };

    let runtime_path;
    if parsed_args.set_runtime_path {
        runtime_path = parsed_args.runtime_path.clone().unwrap();
    } else {
        runtime_path = shog_studio::utils_get_default_runtime_path();
    }

    if !shog_studio::utils_dir_exists(&runtime_path) {
        std::fs::create_dir(&runtime_path).unwrap();
    }

    redirect_stdout(&runtime_path);

    if parsed_args.help {
        println!("shog - v0.0.0");
        println!("{}", SHOG_HELP);

        std::process::exit(0);
    } else if parsed_args.version {
        println!("shog - v0.0.0");
        println!("{}", SHOG_VERSION);

        std::process::exit(0);
    }

    if !parsed_args.invalid_command.is_none() {
        log(
            ERROR,
            &format!("invalid command: {}", parsed_args.invalid_command.unwrap()),
        );
        std::process::exit(1);
    } else if parsed_args.has_invalid_flag {
        log(
            ERROR,
            &format!("invalid flag: {}", parsed_args.invalid_flag.unwrap()),
        );
        std::process::exit(1);
    }

    parsed_args.command = Some("run".to_string());

    match handle_session(parsed_args) {
        Ok(res) => res,

        Err(err) => {
            log(ERROR, &err);
            std::process::exit(1);
        }
    };
}

fn main() {
    std::thread::spawn(|| {
        start_backend();
    });

    sleep_ms(2000);

    tauri::Builder::default()
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
