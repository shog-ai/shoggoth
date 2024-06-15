use shog_studio::SHOG_HELP;
use shog_studio::SHOG_VERSION;
use shog_studio::handle_session;
use shog_studio::log;
use shog_studio::Args;
use shog_studio::LogType::*;

fn main() {
    let args: Vec<String> = std::env::args().collect();

    let parsed_args: Args = match Args::parse(args) {
        Ok(res) => res,

        Err(err) => {
            log(ERROR, &err);
            std::process::exit(1);
        }
    };

    if parsed_args.help || parsed_args.no_args {
        println!("shog - v0.0.0");
        println!("{}", SHOG_HELP);

        std::process::exit(0);
    } else if parsed_args.version {
        println!("shog - v0.0.0");
        println!("{}", SHOG_VERSION);

        std::process::exit(0);
    }

    if !parsed_args.has_command {
        log(ERROR, "no command was found in arguments");
        std::process::exit(1);
    } else if !parsed_args.invalid_command.is_none() {
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

    match handle_session(parsed_args) {
        Ok(res) => res,

        Err(err) => {
            log(ERROR, &err);
            std::process::exit(1);
        }
    };
}
