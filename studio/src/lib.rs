use actix_web::rt::task::spawn_blocking;
use actix_web::{HttpRequest, HttpResponse};
use colored::Colorize;
use ethers::core::k256::ecdsa;
use ethers::core::rand::thread_rng;
use ethers::middleware::Middleware;
use ethers::prelude::*;
use ethers::signers::Signer;
use ethers::signers::{coins_bip39::English, MnemonicBuilder};
use include_dir::{include_dir, Dir};
use std::io::Write;
use std::str::FromStr;
use std::sync::Arc;
use LogType::*;

static STATIC_DIR: Dir = include_dir!("$CARGO_MANIFEST_DIR/src/static");
static TEMPLATES_DIR: Dir = include_dir!("$CARGO_MANIFEST_DIR/src/templates");

pub const SHOG_HELP: &str = r#"
USAGE:
  shog [COMMAND] [<args>] [OPTIONS]...

OPTIONS:
  -h --help                Show this screen.
  -v --version             Show installed version.
  -r <directory>           Specifies a runtime directory

COMMANDS:
  run                      Run shog

EXAMPLES:
  shog run
"#;

pub const SHOG_VERSION: &str = r#"
Part of the Shoggoth Project - https://shog.ai
Copyright (c) 2023 ShogAI
shog comes with absolutely NO WARRANTY of any kind
You may redistribute copies of Shoggoth under the MIT License.
"#;

#[derive(Clone, serde::Serialize)]
pub struct Args {
    pub help: bool,
    pub version: bool,
    pub set_config: bool,
    pub config_path: Option<String>,
    pub no_args: bool,
    pub has_invalid_flag: bool,
    pub invalid_flag: Option<String>,
    pub set_runtime_path: bool,
    pub runtime_path: Option<String>,

    pub has_command: bool,
    pub command: Option<String>,
    pub invalid_command: Option<String>,
    pub has_command_arg: bool,
    pub command_arg: Option<String>,
    pub has_subcommand_arg: bool,
    pub subcommand_arg: Option<String>,
}

const VERSION: &str = "1.0.2";
const HUB_URL: &str = "https://hub.shog.ai";
const NODE_REFRESH_RATE: u64 = 100;

const ERC20_ABI: &str = r#"[{"constant":true,"inputs":[],"name":"name","outputs":[{"name":"","type":"string"}],"payable":false,"stateMutability":"view","type":"function"},{"constant":false,"inputs":[{"name":"spender","type":"address"},{"name":"value","type":"uint256"}],"name":"approve","outputs":[{"name":"","type":"bool"}],"payable":false,"stateMutability":"nonpayable","type":"function"},{"constant":true,"inputs":[],"name":"totalSupply","outputs":[{"name":"","type":"uint256"}],"payable":false,"stateMutability":"view","type":"function"},{"constant":false,"inputs":[{"name":"sender","type":"address"},{"name":"recipient","type":"address"},{"name":"amount","type":"uint256"}],"name":"transferFrom","outputs":[{"name":"","type":"bool"}],"payable":false,"stateMutability":"nonpayable","type":"function"},{"constant":true,"inputs":[],"name":"decimals","outputs":[{"name":"","type":"uint8"}],"payable":false,"stateMutability":"view","type":"function"},{"constant":false,"inputs":[{"name":"spender","type":"address"},{"name":"addedValue","type":"uint256"}],"name":"increaseAllowance","outputs":[{"name":"","type":"bool"}],"payable":false,"stateMutability":"nonpayable","type":"function"},{"constant":false,"inputs":[{"name":"value","type":"uint256"}],"name":"burn","outputs":[],"payable":false,"stateMutability":"nonpayable","type":"function"},{"constant":true,"inputs":[{"name":"account","type":"address"}],"name":"balanceOf","outputs":[{"name":"","type":"uint256"}],"payable":false,"stateMutability":"view","type":"function"},{"constant":true,"inputs":[],"name":"symbol","outputs":[{"name":"","type":"string"}],"payable":false,"stateMutability":"view","type":"function"},{"constant":false,"inputs":[{"name":"spender","type":"address"},{"name":"subtractedValue","type":"uint256"}],"name":"decreaseAllowance","outputs":[{"name":"","type":"bool"}],"payable":false,"stateMutability":"nonpayable","type":"function"},{"constant":false,"inputs":[{"name":"recipient","type":"address"},{"name":"amount","type":"uint256"}],"name":"transfer","outputs":[{"name":"","type":"bool"}],"payable":false,"stateMutability":"nonpayable","type":"function"},{"constant":true,"inputs":[{"name":"owner","type":"address"},{"name":"spender","type":"address"}],"name":"allowance","outputs":[{"name":"","type":"uint256"}],"payable":false,"stateMutability":"view","type":"function"},{"inputs":[{"name":"name","type":"string"},{"name":"symbol","type":"string"},{"name":"decimals","type":"uint8"},{"name":"totalSupply","type":"uint256"},{"name":"feeReceiver","type":"address"},{"name":"tokenOwnerAddress","type":"address"}],"payable":true,"stateMutability":"payable","type":"constructor"},{"anonymous":false,"inputs":[{"indexed":true,"name":"from","type":"address"},{"indexed":true,"name":"to","type":"address"},{"indexed":false,"name":"value","type":"uint256"}],"name":"Transfer","type":"event"},{"anonymous":false,"inputs":[{"indexed":true,"name":"owner","type":"address"},{"indexed":true,"name":"spender","type":"address"},{"indexed":false,"name":"value","type":"uint256"}],"name":"Approval","type":"event"}]"#;
const CROWDSALE_ABI: &str = r#"[{"constant": true,"inputs": [],"name": "rate","outputs": [{	"name": "",	"type": "uint256"}],"payable": false,"stateMutability": "view","type": "function"},	{"constant": true,"inputs": [],"name": "weiRaised","outputs": [	{"name": "","type": "uint256"}],"payable": false,"stateMutability": "view","type": "function"},{"constant": true,"inputs": [],"name": "wallet","outputs": [{"name": "","type": "address"}],"payable": false,"stateMutability": "view","type": "function"},{"constant": false,"inputs": [{"name": "beneficiary","type": "address"}],"name": "buyTokens","outputs": [],"payable": true,"stateMutability": "payable","type": "function"},{"constant": true,"inputs": [],"name": "token","outputs": [{"name": "","type": "address"}],"payable": false,"stateMutability": "view","type": "function"},{"inputs": [{"name": "rate","type": "uint256"},{"name": "wallet","type": "address"},{"name": "token","type": "address"}],"payable": false,"stateMutability": "nonpayable","type": "constructor"},{"payable": true,"stateMutability": "payable","type": "fallback"},{"anonymous": false,"inputs": [{"indexed": true,"name": "purchaser","type": "address"},{"indexed": true,"name": "beneficiary","type": "address"},{"indexed": false,"name": "value","type": "uint256"},{"indexed": false,"name": "amount","type": "uint256"}],"name": "TokensPurchased","type": "event"}]"#;

const MAINNET_CHAIN_ID: u64 = 1u64;
const BASE_CHAIN_ID: u64 = 8453u64;

const HUB_PING_RATE: u64 = 60000; // 1 minute

#[derive(Clone, serde::Serialize, serde::Deserialize)]
struct ConfigNetwork {
    host: String,
    port: u16,
}

impl ConfigNetwork {
    fn new() -> Self {
        Self {
            host: String::from("127.0.0.1"),
            port: 6968,
        }
    }
}

#[derive(Clone, serde::Serialize, serde::Deserialize)]
struct Config {
    network: ConfigNetwork,
}

impl Config {
    fn new() -> Self {
        Self {
            network: ConfigNetwork::new(),
        }
    }
}

#[derive(serde::Serialize, serde::Deserialize, Clone, PartialEq)]
enum ResourceStatus {
    Pending,
    Downloading,
    Paused,
    Seeding,
}

#[derive(serde::Serialize, serde::Deserialize, Clone, PartialEq)]
enum ResourceCategory {
    Model,
    Code,
    Paper,
    Dataset,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
struct Resource {
    status: ResourceStatus,
    category: ResourceCategory,
    download_progress: String,
    downloaded_bytes: u64,
    uploaded_bytes: u64,

    namespace: String,
    name: String,
    shoggoth_id: String,
    torrent: String,
    torrent_id: usize,
    size: u64,
    time_published: i64,
    tags: Vec<String>,

    is_model: bool,
    is_code: bool,
    is_dataset: bool,
    is_paper: bool,

    is_text_generation: bool,

    should_delete: bool,
    should_pause: bool,
    should_unpause: bool,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
struct ChatMessage {
    role: String,
    content: String,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
struct ChatSession {
    name: String,
    messages: Vec<ChatMessage>,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
struct ChatState {
    mount_status: ChatMountStatus,
    namespace: String,
    name: String,

    sessions: Vec<ChatSession>,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
enum ChatMountStatus {
    Mounted,
    Unmounted,
    Mounting,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
struct Settings {
    chat_system_prompt: String,
    shog_hub_url: String,
    eth_rpc: String,
    base_rpc: String,
    shog_contract_eth: String,
    shog_contract_base: String,
    crowdsale_contract_eth: String,
    crowdsale_contract_base: String,
}

struct Ctx {
    chat: ChatState,
    settings: Settings,
    args: Option<Args>,
    runtime_path: String,
    config: Config,
    model_server_process: Option<std::process::Child>,
    version_outdated: bool,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
enum NodeStatus {
    Online,
    Offline,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
struct NodeStats {
    // vec<timestamp, value>
    total_nodes: Vec<(u64, u64)>,
    online_nodes: Vec<(u64, u64)>,
    storage_used: Vec<(u64, u64)>,
    download_bandwidth: Vec<(u64, u64)>,
    upload_bandwidth: Vec<(u64, u64)>,
}

struct NodeCtx {
    node_id: String,
    node_status: NodeStatus,
    node_stats: NodeStats,

    resources: Vec<Resource>,
    torrent_session: Option<Arc<librqbit::Session>>,
    torrent_handles: std::collections::HashMap<String, Arc<librqbit::ManagedTorrent>>,

    wallet: Option<ethers::signers::LocalWallet>,
    wallet_address: String,
}

#[derive(serde::Serialize, serde::Deserialize, Clone)]
struct SaveState {
    // node ctx
    resources: Vec<Resource>,
    node_id: String,
    node_status: NodeStatus,
    node_stats: NodeStats,

    // ctx
    config: Config,
    chat: ChatState,
    settings: Settings,
}

impl Args {
    pub fn new() -> Self {
        Self {
            help: false,
            version: false,
            set_config: false,
            config_path: None,
            no_args: false,
            has_invalid_flag: false,
            invalid_flag: None,
            set_runtime_path: false,
            runtime_path: None,

            has_command: false,
            command: None,
            invalid_command: None,
            has_command_arg: false,
            command_arg: None,
            has_subcommand_arg: false,
            subcommand_arg: None,
        }
    }

    pub fn parse(args: Vec<String>) -> Result<Self, String> {
        let mut result = Self::new();

        let count = args.len();

        let mut index = 1;
        while index < count {
            let current_arg = args.get(index).unwrap();

            if current_arg.chars().next().unwrap() == '-' {
                if current_arg == "--help" || current_arg == "-h" {
                    result.help = true;
                } else if current_arg == "--version" || current_arg == "-v" {
                    result.version = true;
                } else if current_arg == "-c" {
                    result.set_config = true;

                    if index + 1 < count {
                        result.config_path = args.get(index + 1).cloned();
                        index += 1;
                    } else {
                        return Err(String::from("No file was passed to -c flag"));
                    }
                } else if current_arg == "-r" {
                    result.set_runtime_path = true;

                    if index + 1 < count {
                        let runtime_path = args.get(index + 1).cloned().unwrap();
                        index += 1;

                        match std::fs::canonicalize(&runtime_path) {
                            Ok(realpath) => {
                                result.runtime_path =
                                    Some(realpath.as_os_str().to_str().unwrap().to_string());
                            }
                            Err(_) => {
                                log(INFO, &format!("{}", runtime_path.clone()));
                                return Err(String::from("canonicalize path failed"));
                            }
                        }
                    } else {
                        return Err(String::from("No path was passed to -r flag"));
                    }
                } else {
                    result.has_invalid_flag = true;
                    result.invalid_flag = Some(current_arg.clone());
                }
            } else {
                result.has_command = true;
                result.command = Some(current_arg.clone());

                if index + 1 < count && args.get(index + 1).unwrap().chars().next().unwrap() != '-'
                {
                    result.has_command_arg = true;
                    result.command_arg = Some(args.get(index + 1).unwrap().clone());

                    if index + 2 < count
                        && args.get(index + 1).unwrap().chars().next().unwrap() != '-'
                    {
                        result.has_subcommand_arg = true;
                        result.subcommand_arg = Some(args.get(index + 2).unwrap().clone());

                        index += 1;
                    }

                    index += 1;
                }
            }

            index += 1;
        }

        if count == 1 {
            result.no_args = true;
        }

        return Ok(result);
    }
}

impl Resource {
    fn new(
        namespace: String,
        name: String,
        category: ResourceCategory,
        shoggoth_id: String,
    ) -> Self {
        let mut res = Self {
            namespace,
            name,
            status: ResourceStatus::Pending,
            category,
            download_progress: String::from("0.00%"),
            downloaded_bytes: 0,
            uploaded_bytes: 0,
            shoggoth_id,

            torrent: String::new(),
            torrent_id: 0,
            size: 0,
            time_published: 0,
            tags: Vec::new(),

            is_model: false,
            is_code: false,
            is_paper: false,
            is_dataset: false,

            is_text_generation: false,

            should_delete: false,
            should_pause: false,
            should_unpause: false,
        };

        if res.category == ResourceCategory::Model {
            res.is_model = true;
        } else if res.category == ResourceCategory::Code {
            res.is_code = true;
        } else if res.category == ResourceCategory::Dataset {
            res.is_dataset = true;
        } else if res.category == ResourceCategory::Paper {
            res.is_paper = true;
        }

        return res;
    }

    fn from_hub_str(str: &str) -> Self {
        let resource_json: serde_json::Value = serde_json::from_str(str).unwrap();

        let category_str = resource_json.get("category").unwrap().as_str().unwrap();

        let mut category = ResourceCategory::Model;

        if category_str == "Model" {
            category = ResourceCategory::Model;
        } else if category_str == "Code" {
            category = ResourceCategory::Code;
        } else if category_str == "Dataset" {
            category = ResourceCategory::Dataset;
        } else if category_str == "Paper" {
            category = ResourceCategory::Paper;
        }

        let shoggoth_id = resource_json
            .get("shoggoth_id")
            .unwrap()
            .as_str()
            .unwrap()
            .to_string();
        let namespace = resource_json
            .get("namespace")
            .unwrap()
            .as_str()
            .unwrap()
            .to_string();
        let name = resource_json
            .get("name")
            .unwrap()
            .as_str()
            .unwrap()
            .to_string();
        let mut resource = Self::new(namespace, name, category, shoggoth_id);

        let torrent = resource_json
            .get("torrent")
            .unwrap()
            .as_str()
            .unwrap()
            .to_string();
        resource.torrent = torrent;

        let size = resource_json.get("size").unwrap().as_i64().unwrap();
        resource.size = size as u64;

        let time_published = resource_json
            .get("time_published")
            .unwrap()
            .as_i64()
            .unwrap();
        resource.time_published = time_published;

        let tags = resource_json.get("tags").unwrap().as_array().unwrap();

        for tag in tags {
            resource.tags.push(tag.as_str().unwrap().to_string());
        }

        resource
    }

    async fn from_hub(shoggoth_id: &str) -> Self {
        let id = shoggoth_id.to_string();
        let body = spawn_blocking(move || {
            let resp = reqwest::blocking::get(format!("{HUB_URL}/api/get_resource/{id}")).unwrap();

            if resp.status() != reqwest::StatusCode::OK {
                panic!(
                    "resource not found in shog hub: {id} - {}",
                    resp.status().to_string()
                );
            }

            resp.text().unwrap()
        })
        .await
        .unwrap();

        let resource = Resource::from_hub_str(&body);

        resource
    }
}

impl ChatState {
    fn new() -> Self {
        Self {
            mount_status: ChatMountStatus::Unmounted,
            namespace: String::new(),
            name: String::new(),

            sessions: vec![],
        }
    }
}

impl Settings {
    fn new() -> Self {
        Self {
            chat_system_prompt: String::from("You are a helpful assistant."),
            shog_hub_url: String::from("https://hub.shog.ai"),

            eth_rpc: String::from("https://eth.llamarpc.com"),
            base_rpc: String::from("https://base.llamarpc.com"),

            shog_contract_eth: String::from("0xc8388e437031B09B2c61FC4277469091382A1B13"),
            shog_contract_base: String::from("0x6a4F69Da1E2fb2a9b11D1AAD60d03163fE567732"),

            crowdsale_contract_eth: String::from("0x043f7B954Cde1D3AB6607890F85500591bDf0200"),
            crowdsale_contract_base: String::from("0xc8388e437031B09B2c61FC4277469091382A1B13"),
        }
    }
}

impl Ctx {
    fn new() -> Self {
        Self {
            chat: ChatState::new(),
            settings: Settings::new(),
            args: None,
            config: Config::new(),
            runtime_path: String::new(),
            model_server_process: None,
            version_outdated: false,
        }
    }
}

lazy_static::lazy_static! {
    static ref CTX: std::sync::Mutex<Ctx> = std::sync::Mutex::new(Ctx::new());
}

impl NodeStats {
    fn new() -> Self {
        Self {
            total_nodes: vec![],
            online_nodes: vec![],
            storage_used: vec![],
            download_bandwidth: vec![],
            upload_bandwidth: vec![],
        }
    }
}

impl NodeCtx {
    fn new() -> Self {
        Self {
            node_id: String::from("SHOGN"),
            node_status: NodeStatus::Online,
            node_stats: NodeStats::new(),

            resources: Vec::new(),
            torrent_session: None,
            torrent_handles: std::collections::HashMap::new(),

            wallet: None,
            wallet_address: String::new(),
        }
    }
}

lazy_static::lazy_static! {
    static ref NODE_CTX: std::sync::Mutex<NodeCtx> = std::sync::Mutex::new(NodeCtx::new());
}

impl SaveState {
    fn from(ctx: &Ctx, nctx: &NodeCtx) -> Self {
        Self {
            // node ctx
            resources: nctx.resources.clone(),
            node_id: nctx.node_id.clone(),
            node_status: nctx.node_status.clone(),
            node_stats: nctx.node_stats.clone(),

            // ctx
            config: ctx.config.clone(),
            chat: ctx.chat.clone(),
            settings: ctx.settings.clone(),
        }
    }
}

pub fn utils_get_default_runtime_path() -> String {
    let path = match home::home_dir() {
        Some(path) => path,
        None => {
            panic!("could not get home dir");
        }
    };

    let path_str = path.as_os_str().to_str().unwrap().to_string();

    return format!("{}/shog", path_str);
}

pub fn utils_dir_exists(path: &String) -> bool {
    return std::path::Path::new(path).exists() && std::path::Path::new(path).is_dir();
}

fn utils_file_exists(path: &String) -> bool {
    return std::path::Path::new(path).exists() && std::path::Path::new(path).is_file();
}

fn extract_directory(dir: &Dir, destination: &std::path::Path) -> std::io::Result<()> {
    // Iterate over files in the directory
    for file in dir.files() {
        let dest_path = destination.join(file.path().strip_prefix(dir.path()).unwrap());
        if let Some(parent) = dest_path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        let mut dest_file = std::fs::File::create(dest_path)?;
        dest_file.write_all(file.contents())?;
    }

    // Recursively extract subdirectories
    for subdir in dir.dirs() {
        let subdir_path = destination.join(subdir.path().strip_prefix(dir.path()).unwrap());
        extract_directory(subdir, &subdir_path)?;
    }

    Ok(())
}

fn verify_runtime(runtime_path: &String) -> Result<u8, std::io::Error> {
    if !utils_dir_exists(runtime_path) {
        std::fs::create_dir(runtime_path).unwrap();
    }

    let resources_path = format!("{}/resources", runtime_path);
    if !utils_dir_exists(&resources_path) {
        std::fs::create_dir(&resources_path).unwrap();
    }

    let keys_path = format!("{}/.keys", runtime_path);
    if !utils_dir_exists(&keys_path) {
        std::fs::create_dir(&keys_path).unwrap();
    }

    let static_path = format!("{}/static", runtime_path);
    // if !utils_dir_exists(&static_path) {
    extract_directory(&STATIC_DIR, &std::path::Path::new(&static_path))
        .expect("Failed to extract static dir");
    // }

    let templates_path = format!("{}/templates", runtime_path);
    // if !utils_dir_exists(&templates_path) {
    extract_directory(&TEMPLATES_DIR, &std::path::Path::new(&templates_path))
        .expect("Failed to extract templates dir");
    // }

    return Ok(0);
}

pub fn sleep_ms(ms: u64) {
    std::thread::sleep(std::time::Duration::from_millis(ms));
}

fn get_timestamp_ms() -> u64 {
    let start = std::time::SystemTime::now();

    let since_the_epoch = start
        .duration_since(std::time::UNIX_EPOCH)
        .expect("Time went backwards");

    let timestamp_millis = since_the_epoch.as_millis();

    return timestamp_millis as u64;
}

fn generate_wallet(runtime_path: &String) -> ethers::signers::LocalWallet {
    let seed_phrase_path = format!("{}/.keys", runtime_path);

    let mut rng = thread_rng();
    let wallet = MnemonicBuilder::<English>::default()
        .write_to(seed_phrase_path.clone())
        .word_count(12)
        .build_random(&mut rng)
        .unwrap();

    let dir_entries =
        std::fs::read_dir(seed_phrase_path.clone()).expect("Failed to read directory");
    for entry in dir_entries {
        if let Ok(entry) = entry {
            let path = entry.path();

            std::fs::copy(&path, &format!("{seed_phrase_path}/seed.txt")).unwrap();
            std::fs::remove_file(&path).unwrap();

            break;
        }
    }

    return wallet;
}

fn verify_wallet(runtime_path: &String) {
    let mut nctx = NODE_CTX.lock().unwrap();

    let seed_phrase_path = format!("{}/.keys/seed.txt", runtime_path);
    if !utils_file_exists(&seed_phrase_path) {
        let wallet = generate_wallet(runtime_path);
        nctx.wallet = Some(wallet);
    } else {
        let mut phrase_str = std::fs::read_to_string(&seed_phrase_path).unwrap();

        if phrase_str.ends_with('\n') {
            phrase_str.pop();
        }

        let phrase: ethers::core::types::PathOrString =
            ethers::types::PathOrString::String(phrase_str);

        let wallet = MnemonicBuilder::<English>::default()
            .phrase(phrase)
            .build()
            .unwrap();

        nctx.wallet = Some(wallet);
    }

    let address = nctx.wallet.clone().unwrap().address();
    let address_str = format!("0x{:x}", address);

    // log(INFO, &format!("ADDRESS: {}", address_str));

    nctx.wallet_address = address_str.clone();

    nctx.node_id = format!("SHOGN{}", &address_str[2..]);
}

fn init_shog(original_args: Args, print_info: bool) -> Result<u64, String> {
    let mut ctx = CTX.lock().unwrap();
    ctx.args = Some(original_args);

    let args = ctx.args.as_mut().unwrap();

    if args.set_config {
        log(
            INFO,
            &format!(
                "Initializing shog with custom config file: {}",
                args.config_path.clone().unwrap()
            ),
        );
    } else {
        log(INFO, "Initializing shog");
    }

    if args.set_runtime_path {
        ctx.runtime_path = args.runtime_path.clone().unwrap();

        log(
            INFO,
            &format!("Using custom runtime path: {}", ctx.runtime_path.clone()),
        );
    } else {
        let default_runtime_path = utils_get_default_runtime_path();

        ctx.runtime_path = default_runtime_path;

        log(
            INFO,
            &format!("Using default runtime path: {}", ctx.runtime_path.clone()),
        );
    }

    let runtime_path = ctx.runtime_path.clone();
    drop(ctx);

    verify_runtime(&runtime_path).unwrap();

    verify_wallet(&runtime_path);

    if print_info {}

    return Ok(0);
}

fn cook_templates(relative: &str, head_template_data: String, template_data: String) -> String {
    let ctx = CTX.lock().unwrap();
    let runtime_path = ctx.runtime_path.clone();
    drop(ctx);

    let end_path = format!("{}/templates/end.html", runtime_path);
    let end_template_string = std::fs::read_to_string(end_path).unwrap();
    let end_template_data = "{}";
    let end_data_json: serde_json::Value = serde_json::from_str(&end_template_data).unwrap();
    let mut end_template = handlebars::Handlebars::new();
    end_template
        .register_template_string("end", end_template_string)
        .unwrap();
    let end_cooked = end_template.render("end", &end_data_json).unwrap();

    let head_path = format!("{}/templates/head.html", runtime_path);
    let head_template_string = std::fs::read_to_string(head_path).unwrap();
    let head_data_json: serde_json::Value = serde_json::from_str(&head_template_data).unwrap();
    let mut head_template = handlebars::Handlebars::new();
    head_template
        .register_template_string("head", head_template_string)
        .unwrap();
    let head_cooked = head_template.render("head", &head_data_json).unwrap();

    let template_path = format!("{}/templates/{}.html", runtime_path, relative);
    let template_string = std::fs::read_to_string(template_path).unwrap();
    let mut template = handlebars::Handlebars::new();
    template
        .register_template_string("template", template_string)
        .unwrap();
    template.register_escape_fn(handlebars::no_escape);

    let mut data_json: serde_json::Value = serde_json::from_str(&template_data).unwrap();
    data_json
        .as_object_mut()
        .unwrap()
        .insert(String::from("head"), serde_json::json!(head_cooked));
    data_json
        .as_object_mut()
        .unwrap()
        .insert(String::from("end"), serde_json::json!(end_cooked));

    let cooked = template.render("template", &data_json).unwrap();

    return cooked;
}

async fn chat_route(_req: HttpRequest) -> HttpResponse {
    let head_template_data = "{\"title\": \"Shog Studio | Chat\", \"is_chat\": true}";
    let data_string = "{}".to_string();
    let cooked = cook_templates("chat", head_template_data.to_string(), data_string);

    HttpResponse::Ok().body(cooked)
}

async fn hub_route(_req: HttpRequest) -> HttpResponse {
    let head_template_data = serde_json::json!({
        "title": "Shog Studio | Hub",
        "is_hub": true,
        "hub_url": HUB_URL,
        "tags": [
            "llm",
            "gguf",
            "llama",
            "mistral",
            "fineweb",
            "7B",
            "8B",
            "70B",
            "8x7B",
            "llama3",
            "mixture of experts",
            "text generation",
            "image generation",
            "text to image",
            "text to audio",
            "tasks",
            "MOE",
            "mixtral",
            "stable diffusion",
            "StableDiffusionXLPipeline",
        ],
        "publishers": [
            "meta",
            "mistralai",
            "stabilityai",
            "huggingface",
            "karpathy",
            "lighteval",
            "teknium",
            "tensorflow",
            "tinygrad",
            "ggerganov",
            "mozilla-ocho",
            "openinterpreter",
            "pytorch",
            "corcelio",
            "allenai",
            "2noise",
        ]
    });

    let mut template_data_json: serde_json::Value = serde_json::json!({});

    template_data_json
        .as_object_mut()
        .unwrap()
        .insert("is_home".to_string(), serde_json::json!(true));

    let data_string = template_data_json.to_string();

    let cooked = cook_templates("hub", head_template_data.to_string(), data_string);

    HttpResponse::Ok().body(cooked)
}

async fn hub_resource_route(_req: HttpRequest) -> HttpResponse {
    let head_template_data = "{\"title\": \"Shog Studio | Hub \", \"is_resource\": true}";

    let cooked = cook_templates(
        "hub_resource",
        head_template_data.to_string(),
        "{}".to_string(),
    );

    HttpResponse::Ok().body(cooked)
}

async fn api_get_pinned_resources_route(_req: HttpRequest) -> HttpResponse {
    let nctx = NODE_CTX.lock().unwrap();
    let resources = serde_json::to_string(&nctx.resources).unwrap();
    drop(nctx);

    HttpResponse::Ok().body(resources)
}

async fn node_route(_req: HttpRequest) -> HttpResponse {
    let head_template_data = "{\"title\": \"Shog Studio | Node\", \"is_node\": true}";

    let cooked = cook_templates("node", head_template_data.to_string(), "{}".to_string());

    HttpResponse::Ok().body(cooked)
}

async fn account_route(_req: HttpRequest) -> HttpResponse {
    let head_template_data = "{\"title\": \"Shog Studio | Account\", \"is_account\": true}";
    let data_string = "{}".to_string();
    let cooked = cook_templates("account", head_template_data.to_string(), data_string);

    HttpResponse::Ok().body(cooked)
}

async fn leaderboards_route(_req: HttpRequest) -> HttpResponse {
    let head_template_data =
        "{\"title\": \"Shog Studio | Leaderboards\", \"is_leaderboards\": true}";
    let data_string = "{}".to_string();
    let cooked = cook_templates("leaderboards", head_template_data.to_string(), data_string);

    HttpResponse::Ok().body(cooked)
}

async fn settings_route(_req: HttpRequest) -> HttpResponse {
    let head_template_data = "{\"title\": \"Shog Studio | Settings\", \"is_settings\": true}";
    let data_string = "{}".to_string();
    let cooked = cook_templates("settings", head_template_data.to_string(), data_string);

    HttpResponse::Ok().body(cooked)
}

async fn api_ping_route(_req: HttpRequest) -> HttpResponse {
    HttpResponse::Ok().body("pong")
}

async fn api_node_delete_resource_route(req: HttpRequest) -> HttpResponse {
    let shoggoth_id: String = req
        .match_info()
        .get("shoggoth_id")
        .unwrap()
        .parse()
        .unwrap();

    let mut nctx = NODE_CTX.lock().unwrap();

    for resource in &mut nctx.resources {
        if resource.shoggoth_id == shoggoth_id {
            resource.should_delete = true;
        }
    }

    drop(nctx);

    HttpResponse::Ok().body("OK")
}

async fn api_node_pause_resource_route(req: HttpRequest) -> HttpResponse {
    let shoggoth_id: String = req
        .match_info()
        .get("shoggoth_id")
        .unwrap()
        .parse()
        .unwrap();

    let mut nctx = NODE_CTX.lock().unwrap();

    for resource in &mut nctx.resources {
        if resource.shoggoth_id == shoggoth_id {
            resource.should_pause = true;
            resource.status = ResourceStatus::Paused;
        }
    }

    drop(nctx);

    HttpResponse::Ok().body("OK")
}

async fn api_node_unpause_resource_route(req: HttpRequest) -> HttpResponse {
    let shoggoth_id: String = req
        .match_info()
        .get("shoggoth_id")
        .unwrap()
        .parse()
        .unwrap();

    let mut nctx = NODE_CTX.lock().unwrap();

    for resource in &mut nctx.resources {
        if resource.shoggoth_id == shoggoth_id {
            resource.should_unpause = true;

            if resource.download_progress == "100.00%" {
                resource.status = ResourceStatus::Seeding;
            } else {
                resource.status = ResourceStatus::Downloading;
            }
        }
    }

    drop(nctx);

    HttpResponse::Ok().body("OK")
}

async fn api_node_add_resource_route(req: HttpRequest, _req_body: String) -> HttpResponse {
    let shoggoth_id: String = req
        .match_info()
        .get("shoggoth_id")
        .unwrap()
        .parse()
        .unwrap();

    let resource = Resource::from_hub(&shoggoth_id).await;

    log(
        INFO,
        &format!("adding resource: {}/{}", resource.namespace, resource.name),
    );

    let mut nctx = NODE_CTX.lock().unwrap();
    nctx.resources.push(resource);
    drop(nctx);

    HttpResponse::Ok().body("OK")
}

async fn api_leaderboard_register_vote_route(req: HttpRequest) -> HttpResponse {
    let category: String = req.match_info().get("category").unwrap().parse().unwrap();
    let namespace: String = req.match_info().get("namespace").unwrap().parse().unwrap();
    let name: String = req.match_info().get("name").unwrap().parse().unwrap();

    // log(
    // INFO,
    // &format!("registering vote for {category} category -> {namespace}/{name}"),
    // );

    let mut nctx = NODE_CTX.lock().unwrap();

    let node_id = nctx.node_id.clone();

    let payload = serde_json::json!({"node_id": node_id, "category": category, "namespace": namespace, "name": name}).to_string();

    let signature = wallet_sign_string(&mut nctx, &payload).await;

    drop(nctx);

    // log(INFO, &format!("SIGNATURE: {signature}"));

    let body = serde_json::json!(
        {
            "payload": payload,
            "signature": signature
        }
    );

    let resp = reqwest::Client::new()
        .post(format!("{HUB_URL}/api/leaderboard_register_vote"))
        .json(&body)
        .send()
        .await;

    if resp.is_err() {
        log(ERROR, "Hub unreachable");
    } else if !resp.as_ref().unwrap().status().is_success() {
        let err = resp.unwrap().text().await.unwrap();

        log(WARN, &format!("hub register_vote response not OK: {}", err));

        return HttpResponse::BadRequest().body(err);
    }

    HttpResponse::Ok().body("OK")
}

async fn api_update_settings_route(_req: HttpRequest, req_body: String) -> HttpResponse {
    let mut ctx = CTX.lock().unwrap();
    ctx.settings = serde_json::from_str(&req_body).unwrap();
    drop(ctx);

    HttpResponse::Ok().body("OK")
}

async fn api_get_chat_state_route(_req: HttpRequest) -> HttpResponse {
    let nctx = NODE_CTX.lock().unwrap();
    let resources = nctx.resources.clone();
    drop(nctx);

    let mut downloaded_models = serde_json::json!([]);

    let downloaded_models = downloaded_models.as_array_mut().unwrap();

    for resource in resources {
        if resource.category == ResourceCategory::Model
            && resource.status == ResourceStatus::Seeding
        {
            for tag in &resource.tags {
                if tag == "gguf" {
                    downloaded_models.push(serde_json::to_value(resource).unwrap());
                    break;
                }
            }
        }
    }

    let ctx = CTX.lock().unwrap();

    let state = serde_json::json!({
        "models": downloaded_models,
        "state": ctx.chat
    });

    HttpResponse::Ok().json(state)
}

async fn api_chat_add_session_route(_req: HttpRequest, body: String) -> HttpResponse {
    let mut ctx = CTX.lock().unwrap();

    let body_json: serde_json::Value = serde_json::from_str(&body).unwrap();
    let name = body_json.get("name").unwrap().as_str().unwrap().to_string();

    ctx.chat.sessions.push(ChatSession {
        name,
        messages: vec![],
    });

    HttpResponse::Ok().finish()
}

async fn api_chat_delete_session_route(req: HttpRequest) -> HttpResponse {
    let index: u64 = req.match_info().get("index").unwrap().parse().unwrap();

    let mut ctx = CTX.lock().unwrap();

    ctx.chat.sessions.remove(index as usize);

    HttpResponse::Ok().finish()
}

async fn api_chat_add_message_route(_req: HttpRequest, body: String) -> HttpResponse {
    let mut ctx = CTX.lock().unwrap();

    let body_json: serde_json::Value = serde_json::from_str(&body).unwrap();

    let index = body_json.get("session_index").unwrap().as_i64().unwrap();
    let role = body_json.get("role").unwrap().as_str().unwrap().to_string();
    let content = body_json
        .get("content")
        .unwrap()
        .as_str()
        .unwrap()
        .to_string();

    if ctx.chat.sessions[index as usize].messages.len() == 0 {
        ctx.chat.sessions[index as usize].name = if content.len() > 20 {
            format!("{}...", &content[..20])
        } else {
            content.to_string()
        };

        let system_prompt = ctx.settings.chat_system_prompt.clone();

        ctx.chat.sessions[index as usize]
            .messages
            .push(ChatMessage {
                role: "system".to_string(),
                content: system_prompt,
            });
    }

    ctx.chat.sessions[index as usize]
        .messages
        .push(ChatMessage { role, content });

    HttpResponse::Ok().finish()
}

async fn api_get_node_info_route(_req: HttpRequest) -> HttpResponse {
    let nctx = NODE_CTX.lock().unwrap();
    let id = nctx.node_id.clone();
    let status = nctx.node_status.clone();
    drop(nctx);

    let res = serde_json::json!({
        "node_id": id,
        "status": status
    });

    HttpResponse::Ok().json(res)
}

async fn api_get_node_stats_route(_req: HttpRequest) -> HttpResponse {
    let nctx = NODE_CTX.lock().unwrap();
    let stats = nctx.node_stats.clone();
    drop(nctx);

    HttpResponse::Ok().json(stats)
}

fn mount_model() {
    let ctx = CTX.lock().unwrap();
    let runtime_path = ctx.runtime_path.clone();
    let model_path = format!(
        "{}/resources/{}-{}/{}",
        runtime_path, ctx.chat.namespace, ctx.chat.name, ctx.chat.name
    );
    drop(ctx);

    let child_process = std::process::Command::new("python3")
        .arg("-m")
        .arg("llama_cpp.server")
        .arg("--model")
        .arg(model_path)
        .current_dir(runtime_path)
        .stdout(std::process::Stdio::piped()) // Redirect child process's stdout to a pipe
        .spawn()
        .expect("Failed to start child process");

    sleep_ms(2000);

    // if let Some(mut stdout) = child_process.stdout.take() {
    // std::io::copy(&mut stdout, &mut std::io::stdout())
    // .expect("Failed to read child process stdout");
    // }

    let mut ctx = CTX.lock().unwrap();
    ctx.model_server_process = Some(child_process);
}

async fn api_mount_model_route(req: HttpRequest) -> HttpResponse {
    let name: String = req.match_info().get("name").unwrap().parse().unwrap();

    log(INFO, &format!("mounting model: {name}"));

    let nctx = NODE_CTX.lock().unwrap();
    let resources = nctx.resources.clone();
    drop(nctx);

    let mut ctx = CTX.lock().unwrap();

    for resource in resources {
        if resource.name == name
            && resource.category == ResourceCategory::Model
            && resource.status == ResourceStatus::Seeding
        {
            ctx.chat.namespace = resource.namespace.clone();
            ctx.chat.name = name.clone();
            ctx.chat.mount_status = ChatMountStatus::Mounting;
        }
    }

    drop(ctx);

    mount_model();

    log(INFO, &format!("model mounted"));

    let mut ctx = CTX.lock().unwrap();
    ctx.chat.mount_status = ChatMountStatus::Mounted;

    HttpResponse::Ok().body("OK")
}

async fn api_is_resource_pinned_route(req: HttpRequest) -> HttpResponse {
    let shoggoth_id: String = req
        .match_info()
        .get("shoggoth_id")
        .unwrap()
        .parse()
        .unwrap();

    let nctx = NODE_CTX.lock().unwrap();

    let mut pinned = false;

    for resource in nctx.resources.iter() {
        if resource.shoggoth_id == shoggoth_id {
            pinned = true;
        }
    }

    HttpResponse::Ok().body(pinned.to_string())
}

async fn api_get_resource_route(req: HttpRequest) -> HttpResponse {
    let shoggoth_id: String = req
        .match_info()
        .get("shoggoth_id")
        .unwrap()
        .parse()
        .unwrap();

    let nctx = NODE_CTX.lock().unwrap();

    for resource in nctx.resources.iter() {
        if resource.shoggoth_id == shoggoth_id {
            let resource_str = serde_json::to_string(&resource).unwrap();

            return HttpResponse::Ok().body(resource_str);
        }
    }

    return HttpResponse::InternalServerError().finish();
}

async fn api_get_resource_by_name_route(req: HttpRequest) -> HttpResponse {
    let namespace: String = req.match_info().get("namespace").unwrap().parse().unwrap();
    let name: String = req.match_info().get("name").unwrap().parse().unwrap();

    let nctx = NODE_CTX.lock().unwrap();

    for resource in nctx.resources.iter() {
        if resource.namespace == namespace && resource.name == name {
            let resource_str = serde_json::to_string(&resource).unwrap();

            return HttpResponse::Ok().body(resource_str);
        }
    }

    return HttpResponse::InternalServerError().finish();
}

// fn wei_to_ether(wei: U256) -> f64 {
// let ether_value = wei.as_u128() as f64 / 1e18;
// ether_value
// }

fn wei_to_ether_str(wei: U256) -> String {
    let ether_value = wei.as_u128() as f64 / 1e18;
    format!("{:.6}", ether_value)
}

async fn get_eth_balance(rpc_url: &str, wallet_address: &str) -> Result<U256, String> {
    let wallet_address = H160::from_str(&wallet_address).unwrap();

    let provider = ethers::providers::Provider::<ethers::providers::Http>::try_from(rpc_url)
        .expect("could not instantiate HTTP Provider");

    let block = provider.get_block_number().await.unwrap();

    let eth_balance = provider
        .get_balance(wallet_address, Some(block.into()))
        .await;

    if eth_balance.is_ok() {
        return Ok(eth_balance.unwrap());
    } else {
        return Err(format!(
            "get eth balance failed - {rpc_url} - {wallet_address}"
        ));
    }
}

async fn get_token_balance(
    rpc_url: &str,
    contract_address: &str,
    wallet_address: &str,
) -> Result<U256, String> {
    let contract_address = H160::from_str(&contract_address).unwrap();

    let wallet_address = H160::from_str(&wallet_address).unwrap();

    let provider = ethers::providers::Provider::<ethers::providers::Http>::try_from(rpc_url)
        .expect("could not instantiate HTTP Provider");

    let abi: ethers::core::abi::Abi = serde_json::from_str(ERC20_ABI).unwrap();

    let contract = ethers::contract::Contract::new(contract_address, abi, Arc::new(provider));

    let balance = contract
        .method::<_, U256>("balanceOf", wallet_address)
        .unwrap()
        .call()
        .await;

    if balance.is_ok() {
        return Ok(balance.unwrap());
    } else {
        return Err(format!(
            "get token balance failed - {rpc_url} - {contract_address} - {wallet_address}"
        ));
    }
}

async fn crowdsale_buy(
    rpc_url: &str,
    crowdsale_contract: &str,
    wallet_address: &str,
    wallet: Wallet<ecdsa::SigningKey>,
    eth_value: U256,
) -> Result<u64, String> {
    let contract_address = H160::from_str(&crowdsale_contract).unwrap();

    let wallet_address = H160::from_str(&wallet_address).unwrap();

    let provider = ethers::providers::Provider::<ethers::providers::Http>::try_from(rpc_url)
        .expect("could not instantiate HTTP Provider");

    let client = SignerMiddleware::new(provider.clone(), wallet);

    let abi: ethers::core::abi::Abi = serde_json::from_str(CROWDSALE_ABI).unwrap();

    let contract = ethers::contract::Contract::new(contract_address, abi, Arc::new(client));

    let tx = contract
        .method::<_, H256>("buyTokens", wallet_address)
        .unwrap()
        .value(eth_value);

    let pending_tx = tx.send().await;
    if !pending_tx.is_ok() {
        return Err("crowdsale buy failed".to_string());
    }

    // Wait for the transaction receipt
    let tx_receipt = pending_tx.unwrap().await;
    if !tx_receipt.is_ok() {
        return Err("crowdsale buy failed".to_string());
    }

    if tx_receipt.unwrap().is_some() {
        return Ok(0);
    } else {
        return Err("crowdsale buy failed".to_string());
    }
}

async fn api_buy_shog_route(req: HttpRequest) -> HttpResponse {
    let chain: String = req.match_info().get("chain").unwrap().parse().unwrap();
    let eth_value: String = req.match_info().get("eth_value").unwrap().parse().unwrap();

    let ctx = CTX.lock().unwrap();

    let eth_rpc = ctx.settings.eth_rpc.clone();
    let base_rpc = ctx.settings.base_rpc.clone();

    let crowdsale_contract_eth = ctx.settings.crowdsale_contract_eth.clone();
    let crowdsale_contract_base = ctx.settings.crowdsale_contract_base.clone();

    drop(ctx);

    let nctx = NODE_CTX.lock().unwrap();
    let wallet_address = nctx.wallet_address.clone();

    let wallet = nctx.wallet.as_ref().unwrap().clone();

    drop(nctx);

    let eth_value = ethers::utils::parse_ether(eth_value).unwrap();

    if chain == "mainnet" {
        let res = crowdsale_buy(
            &eth_rpc,
            &crowdsale_contract_eth,
            &wallet_address,
            wallet.with_chain_id(MAINNET_CHAIN_ID),
            eth_value,
        )
        .await;

        if res.is_ok() {
            return HttpResponse::Ok().finish();
        } else {
            return HttpResponse::InternalServerError().finish();
        }
    } else {
        let res = crowdsale_buy(
            &base_rpc,
            &crowdsale_contract_base,
            &wallet_address,
            wallet.with_chain_id(BASE_CHAIN_ID),
            eth_value,
        )
        .await;

        if res.is_ok() {
            return HttpResponse::Ok().finish();
        } else {
            return HttpResponse::InternalServerError().finish();
        }
    }
}

fn wallet_address_to_node_id(address: &str) -> String {
    return format!("SHOGN{}", &address[2..]);
}

// fn node_id_to_wallet_address(node_id: &str) -> String {
// return format!("0x{}", &node_id[5..]);
// }

async fn api_get_crypto_balance_route(req: HttpRequest) -> HttpResponse {
    let wallet_address: String = req.match_info().get("address").unwrap().parse().unwrap();

    let ctx = CTX.lock().unwrap();

    let eth_rpc = ctx.settings.eth_rpc.clone();
    let base_rpc = ctx.settings.base_rpc.clone();

    let shog_contract_eth = ctx.settings.shog_contract_eth.clone();
    let shog_contract_base = ctx.settings.shog_contract_base.clone();
    drop(ctx);

    let eth_balance_main: U256 = get_eth_balance(&eth_rpc, &wallet_address).await.unwrap();
    let eth_balance_base: U256 = get_eth_balance(&base_rpc, &wallet_address).await.unwrap();

    let shog_balance_main: U256 = get_token_balance(&eth_rpc, &shog_contract_eth, &wallet_address)
        .await
        .unwrap();

    let shog_balance_base: U256 =
        get_token_balance(&base_rpc, &shog_contract_base, &wallet_address)
            .await
            .unwrap();

    let mut rewards_eth: u64 = 0;
    let mut rewards_base: u64 = 0;

    let node_id = wallet_address_to_node_id(&wallet_address);
    let resp = reqwest::get(format!("{HUB_URL}/api/get_node_rewards/{node_id}")).await;
    if resp.is_err() || !resp.as_ref().unwrap().status().is_success() {
        log(ERROR, "Hub unreachable");
    } else {
        let body = resp.unwrap().text().await.unwrap();

        let body_json: serde_json::Value = serde_json::from_str(&body).unwrap();
        rewards_eth = body_json.get("rewards_eth").unwrap().as_i64().unwrap() as u64;
        rewards_base = body_json.get("rewards_base").unwrap().as_i64().unwrap() as u64;
    }

    let res_json = serde_json::json!({
        "eth_main": wei_to_ether_str(eth_balance_main),
        "eth_base": wei_to_ether_str(eth_balance_base),
        "shog_main": wei_to_ether_str(shog_balance_main),
        "shog_base": wei_to_ether_str(shog_balance_base),
        "rewards_eth": rewards_eth,
        "rewards_base": rewards_base,
    });

    return HttpResponse::Ok().json(res_json);
}

async fn api_unmount_model_route(_req: HttpRequest) -> HttpResponse {
    let mut ctx = CTX.lock().unwrap();

    log(INFO, &format!("unmounting model: {}", ctx.chat.name));

    ctx.model_server_process.as_mut().unwrap().kill().unwrap();

    ctx.chat.mount_status = ChatMountStatus::Unmounted;

    HttpResponse::Ok().body("OK")
}

async fn dynamic_const_route(_req: HttpRequest) -> HttpResponse {
    let ctx = CTX.lock().unwrap();
    let settings = serde_json::to_string(&ctx.settings).unwrap();
    let version_outdated = ctx.version_outdated.clone();
    drop(ctx);

    let nctx = NODE_CTX.lock().unwrap();
    let node_id = nctx.node_id.clone();
    let wallet_address = nctx.wallet_address.clone();
    drop(nctx);

    let body = format!(
        r#"
        let version = '{VERSION}';

        let version_outdated = {version_outdated};
        
        let hub_url = '{HUB_URL}';
        
        let api_url = '/api';

        let settings = {settings};

        let node_id = '{node_id}';

        let wallet_address = '{wallet_address}';
    "#
    );

    HttpResponse::Ok()
        .content_type("application/javascript; charset=utf-8")
        .body(body)
}

async fn index_route(_req: HttpRequest) -> HttpResponse {
    HttpResponse::Found()
        .append_header(("Location", "/hub"))
        .finish()
}

async fn api_force_exit_route(_req: HttpRequest) -> HttpResponse {
    log(INFO, "FORCE EXIT CALLED, shutting down...");

    std::process::exit(0);
}

async fn api_open_downloads_folder_route(_req: HttpRequest) -> HttpResponse {
    log(INFO, "opening downloads folder...");

    let ctx = CTX.lock().unwrap();
    let runtime_path = ctx.runtime_path.clone();
    let downloads_path = format!("{}/resources/", runtime_path,);
    drop(ctx);

    opener::open(downloads_path).unwrap();

    HttpResponse::Ok().finish()
}

async fn api_open_seed_phrase_route(_req: HttpRequest) -> HttpResponse {
    log(INFO, "opening seed phrase...");

    let ctx = CTX.lock().unwrap();
    let runtime_path = ctx.runtime_path.clone();
    let downloads_path = format!("{}/.keys/seed.txt", runtime_path,);
    drop(ctx);

    opener::open(downloads_path).unwrap();

    HttpResponse::Ok().finish()
}

async fn api_open_link_route(_req: HttpRequest, body: String) -> HttpResponse {
    log(INFO, &format!("opening link: {body}"));

    opener::open(body).unwrap();

    HttpResponse::Ok().finish()
}

#[actix_web::main]
async fn start_frontend_server() -> std::io::Result<()> {
    let ctx = CTX.lock().unwrap();
    let host = ctx.config.network.host.clone();
    let port = ctx.config.network.port;
    drop(ctx);

    actix_web::HttpServer::new(|| {
        let ctx = CTX.lock().unwrap();
        let static_path = format!("{}/static", ctx.runtime_path);
        drop(ctx);

        actix_web::App::new()
            .service(actix_files::Files::new("/static", &static_path).show_files_listing())
            .route("/", actix_web::web::get().to(index_route))
            .route("/hub", actix_web::web::get().to(hub_route))
            .route(
                "/hub/resource/{namespace}/{resource}",
                actix_web::web::get().to(hub_resource_route),
            )
            .route("/chat", actix_web::web::get().to(chat_route))
            .route("/node", actix_web::web::get().to(node_route))
            .route("/account", actix_web::web::get().to(account_route))
            .route(
                "/leaderboards",
                actix_web::web::get().to(leaderboards_route),
            )
            .route("/settings", actix_web::web::get().to(settings_route))
            //
            .route("/api/ping", actix_web::web::get().to(api_ping_route))
            .route(
                "/api/force_exit",
                actix_web::web::get().to(api_force_exit_route),
            )
            .route(
                "/api/open_downloads_folder",
                actix_web::web::get().to(api_open_downloads_folder_route),
            )
            .route(
                "/api/open_seed_phrase",
                actix_web::web::get().to(api_open_seed_phrase_route),
            )
            .route(
                "/api/open_link",
                actix_web::web::post().to(api_open_link_route),
            )
            .route(
                "/api/get_chat_state",
                actix_web::web::get().to(api_get_chat_state_route),
            )
            .route(
                "/api/chat_add_session",
                actix_web::web::post().to(api_chat_add_session_route),
            )
            .route(
                "/api/chat_delete_session/{index}",
                actix_web::web::get().to(api_chat_delete_session_route),
            )
            .route(
                "/api/chat_add_message",
                actix_web::web::post().to(api_chat_add_message_route),
            )
            .route(
                "/api/node_add_resource/{shoggoth_id}",
                actix_web::web::get().to(api_node_add_resource_route),
            )
            .route(
                "/api/node_delete_resource/{shoggoth_id}",
                actix_web::web::get().to(api_node_delete_resource_route),
            )
            .route(
                "/api/node_pause_resource/{shoggoth_id}",
                actix_web::web::get().to(api_node_pause_resource_route),
            )
            .route(
                "/api/node_unpause_resource/{shoggoth_id}",
                actix_web::web::get().to(api_node_unpause_resource_route),
            )
            .route(
                "/api/leaderboard_register_vote/{category}/{namespace}/{name}",
                actix_web::web::get().to(api_leaderboard_register_vote_route),
            )
            .route(
                "/api/mount_model/{name}",
                actix_web::web::get().to(api_mount_model_route),
            )
            .route(
                "/api/unmount_model",
                actix_web::web::get().to(api_unmount_model_route),
            )
            .route(
                "/api/update_settings",
                actix_web::web::post().to(api_update_settings_route),
            )
            .route(
                "/api/is_resource_pinned/{shoggoth_id}",
                actix_web::web::get().to(api_is_resource_pinned_route),
            )
            .route(
                "/api/get_resource/{shoggoth_id}",
                actix_web::web::get().to(api_get_resource_route),
            )
            .route(
                "/api/get_pinned_resources",
                actix_web::web::get().to(api_get_pinned_resources_route),
            )
            .route(
                "/api/get_resource_by_name/{namespace}/{name}",
                actix_web::web::get().to(api_get_resource_by_name_route),
            )
            .route(
                "/api/get_node_info",
                actix_web::web::get().to(api_get_node_info_route),
            )
            .route(
                "/api/get_crypto_balance/{address}",
                actix_web::web::get().to(api_get_crypto_balance_route),
            )
            .route(
                "/api/get_node_stats",
                actix_web::web::get().to(api_get_node_stats_route),
            )
            .route(
                "/api/buy_shog/{chain}/{eth_value}",
                actix_web::web::get().to(api_buy_shog_route),
            )
            //
            .route(
                "/dynamic/const.js",
                actix_web::web::get().to(dynamic_const_route),
            )
    })
    .bind((host, port))?
    .run()
    .await
}

fn start_server() -> std::thread::JoinHandle<()> {
    let ctx = CTX.lock().unwrap();
    let host = ctx.config.network.host.clone();
    let port = ctx.config.network.port;
    drop(ctx);

    log(INFO, &format!("starting backend at http://{host}:{port}"));

    let handler = std::thread::Builder::new()
        .name("start_frontend_server".to_string())
        .spawn(move || {
            start_frontend_server().unwrap();
        })
        .unwrap();

    return handler;
}

fn delete_resources(nctx: &mut NodeCtx) {
    // free necessary resources
    for resource in &mut nctx.resources {
        if resource.should_delete {
            let handle = nctx.torrent_handles.get(&resource.shoggoth_id);

            if handle.is_some() {
                nctx.torrent_session
                    .as_mut()
                    .unwrap()
                    .delete(resource.torrent_id, true)
                    .unwrap();
                nctx.torrent_handles.remove(&resource.shoggoth_id);
            }
        }
    }

    nctx.resources.retain(|item| !item.should_delete);
}

async fn torrent_download_resource(
    session: Arc<librqbit::Session>,
    resource: &Resource,
) -> (usize, Arc<librqbit::ManagedTorrent>) {
    let sub_folder = format!("{}-{}", resource.namespace.clone(), resource.name.clone());

    let torrent_handle = session
        .add_torrent(
            librqbit::AddTorrent::from_url(&resource.torrent),
            Some(librqbit::AddTorrentOptions {
                paused: false,
                only_files_regex: None,
                only_files: None,
                overwrite: true,
                list_only: false,
                output_folder: None,
                sub_folder: Some(sub_folder),
                peer_opts: None,
                force_tracker_interval: None,
                disable_trackers: false,
                initial_peers: None,
                preferred_id: None,
            }),
        )
        .await
        .unwrap();

    match torrent_handle {
        librqbit::AddTorrentResponse::AlreadyManaged(_, _) => {
            panic!("Alreadymanaged returned from add torrent");
        }

        librqbit::AddTorrentResponse::ListOnly(_) => {
            panic!("Listonly returned from add torrent");
        }

        librqbit::AddTorrentResponse::Added(id, managed) => {
            return (id, managed);
        }
    }
}

async fn begin_downloading(session: Arc<librqbit::Session>, resource: Resource) {
    let res = torrent_download_resource(session, &resource).await;

    let mut nctx = NODE_CTX.lock().unwrap();

    for state_resource in &mut nctx.resources {
        if state_resource.shoggoth_id == resource.shoggoth_id {
            state_resource.torrent_id = res.0;

            nctx.torrent_handles
                .insert(resource.shoggoth_id.clone(), res.1);

            break;
        }
    }
}

async fn poll_resources(nctx: &mut NodeCtx) {
    for resource in &mut nctx.resources {
        if resource.status == ResourceStatus::Pending {
            resource.status = ResourceStatus::Downloading;

            let new_session = nctx.torrent_session.as_mut().unwrap().clone();
            let new_resource = resource.clone();

            tokio::spawn(async move {
                begin_downloading(new_session, new_resource).await;
            });
        } else if resource.status == ResourceStatus::Downloading {
            let handle = nctx.torrent_handles.get(&resource.shoggoth_id);

            if handle.is_some() {
                let progress = handle
                    .unwrap()
                    .stats()
                    .progress_percent_human_readable()
                    .to_string();

                resource.download_progress = progress;

                if resource.download_progress == "100.00%"
                    && resource.status != ResourceStatus::Seeding
                {
                    resource.status = ResourceStatus::Seeding;

                    let resp = reqwest::get(format!(
                        "{HUB_URL}/api/download_complete/{}",
                        resource.shoggoth_id
                    ))
                    .await;

                    if resp.is_err() || !resp.as_ref().unwrap().status().is_success() {
                        log(ERROR, "Hub unreachable");
                    }
                }
            }
        }

        if resource.should_pause {
            let handle = nctx.torrent_handles.get(&resource.shoggoth_id);

            if handle.is_some() {
                let paused = handle.unwrap().pause();

                if paused.is_err() {
                    match paused {
                        Ok(_) => {}

                        Err(err) => {
                            log(ERROR, &format!("{}", err));
                        }
                    }
                }
            }
            resource.should_pause = false;
        } else if resource.should_unpause {
            let handle = nctx.torrent_handles.get(&resource.shoggoth_id);

            if handle.is_some() {
                let unpaused = nctx
                    .torrent_session
                    .as_mut()
                    .unwrap()
                    .unpause(handle.unwrap());

                if unpaused.is_err() {
                    match unpaused {
                        Ok(_) => {}

                        Err(err) => {
                            log(ERROR, &format!("{}", err));
                        }
                    }
                }
            }
            resource.should_unpause = false;
        }
    }
}

#[tokio::main]
async fn start_node_service() {
    let ctx = CTX.lock().unwrap();
    let runtime_path = ctx.runtime_path.clone();
    drop(ctx);

    let mut nctx = NODE_CTX.lock().unwrap();

    let download_path = format!("{runtime_path}/resources");

    let options = librqbit::SessionOptions::default();

    let session = librqbit::Session::new_with_opts(download_path.into(), options)
        .await
        .unwrap();

    nctx.torrent_session = Some(session);

    drop(nctx);

    loop {
        sleep_ms(NODE_REFRESH_RATE);

        let mut nctx = NODE_CTX.lock().unwrap();

        delete_resources(&mut nctx);
        poll_resources(&mut nctx).await;

        drop(nctx);
    }
}

fn data_saver() {
    loop {
        sleep_ms(5000);

        let ctx = CTX.lock().unwrap();
        let nctx = NODE_CTX.lock().unwrap();

        let state = SaveState::from(&ctx, &nctx);
        let state_str = serde_json::to_string(&state).unwrap();

        let save_path = format!("{}/save.sdb", ctx.runtime_path);

        std::fs::write(save_path, state_str).unwrap();
    }
}

fn get_storage_usage() -> u64 {
    let mut total: u64 = 0;

    let nctx = NODE_CTX.lock().unwrap();

    let torrent_handles = nctx.torrent_handles.clone();

    for resource in nctx.resources.iter() {
        let handle = torrent_handles.get(&resource.shoggoth_id);

        if handle.is_some() {
            let downloaded_bytes = handle.unwrap().stats().progress_bytes;

            total += downloaded_bytes;
        }
    }

    return total;
}

fn get_download_bandwidth() -> u64 {
    let mut total: u64 = 0;

    let mut nctx = NODE_CTX.lock().unwrap();

    let torrent_handles = nctx.torrent_handles.clone();

    for resource in nctx.resources.iter_mut() {
        if resource.status == ResourceStatus::Downloading {
            let handle = torrent_handles.get(&resource.shoggoth_id);

            if handle.is_some() {
                let downloaded_bytes = handle.unwrap().stats().progress_bytes;

                if downloaded_bytes > resource.downloaded_bytes {
                    let difference = downloaded_bytes - resource.downloaded_bytes;

                    resource.downloaded_bytes = downloaded_bytes;

                    total += difference;
                }
            }
        }
    }

    return total;
}

fn get_upload_bandwidth() -> u64 {
    let mut total: u64 = 0;

    let mut nctx = NODE_CTX.lock().unwrap();

    let torrent_handles = nctx.torrent_handles.clone();

    for resource in nctx.resources.iter_mut() {
        let handle = torrent_handles.get(&resource.shoggoth_id);

        if handle.is_some() {
            let uploaded_bytes = handle.unwrap().stats().uploaded_bytes;

            if uploaded_bytes > resource.uploaded_bytes {
                let difference = uploaded_bytes - resource.uploaded_bytes;

                resource.uploaded_bytes = uploaded_bytes;

                total += difference;
            }
        }
    }

    return total;
}

fn get_hub_total_nodes() -> Result<u64, String> {
    let resp = reqwest::blocking::get(format!("{HUB_URL}/api/get_total_nodes"));

    if resp.is_err() || !resp.as_ref().unwrap().status().is_success() {
        log(ERROR, "Hub unreachable");

        return Err("hub unreachable".to_string());
    } else {
        let body = resp.unwrap().text().unwrap();

        let body_json: serde_json::Value = serde_json::from_str(&body).unwrap();
        let count = body_json.get("count").unwrap().as_i64().unwrap();

        return Ok(count as u64);
    }
}

fn get_hub_online_nodes() -> Result<u64, String> {
    let resp = reqwest::blocking::get(format!("{HUB_URL}/api/get_online_nodes"));

    if resp.is_err() || !resp.as_ref().unwrap().status().is_success() {
        log(ERROR, "Hub unreachable");

        return Err("hub unreachable".to_string());
    } else {
        let body = resp.unwrap().text().unwrap();

        let body_json: serde_json::Value = serde_json::from_str(&body).unwrap();
        let count = body_json.get("count").unwrap().as_i64().unwrap();

        return Ok(count as u64);
    }
}

async fn wallet_sign_string(nctx: &mut NodeCtx, str: &str) -> String {
    let signature = nctx
        .wallet
        .as_mut()
        .unwrap()
        .sign_message(str)
        .await
        .unwrap()
        .to_string();

    return signature;
}

fn ping_hub() {
    let nctx = NODE_CTX.lock().unwrap();
    let node_id = nctx.node_id.clone();
    drop(nctx);

    let timestamp = get_timestamp_ms();

    let payload = serde_json::json!({"node_id": node_id, "timestamp": timestamp}).to_string();

    let mut nctx = NODE_CTX.lock().unwrap();
    let signature = tokio::runtime::Runtime::new()
        .unwrap()
        .block_on(wallet_sign_string(&mut nctx, &payload));
    drop(nctx);

    // log(INFO, &format!("SIGNATURE: {signature}"));

    let body = serde_json::json!(
        {
            "payload": payload,
            "signature": signature
        }
    );

    let resp = reqwest::blocking::Client::new()
        .post(format!("{HUB_URL}/api/node_hub_ping"))
        .json(&body)
        .send();

    if resp.is_err() {
        log(ERROR, "Hub unreachable");
        let mut nctx = NODE_CTX.lock().unwrap();
        nctx.node_status = NodeStatus::Offline;
    } else if !resp.as_ref().unwrap().status().is_success() {
        let err = resp.unwrap().text().unwrap();

        log(WARN, &format!("hub ping response not OK: {}", err));
        let mut nctx = NODE_CTX.lock().unwrap();
        nctx.node_status = NodeStatus::Offline;
    } else {
        let body = resp.unwrap().text().unwrap();
        let body_json: serde_json::Value = serde_json::from_str(&body).unwrap();

        let version = body_json.get("version").unwrap().as_str().unwrap();

        if version != VERSION {
            let mut ctx = CTX.lock().unwrap();
            ctx.version_outdated = true;
            log(
                WARN,
                &format!("version {VERSION} outdated, update to the latest version {version}"),
            );
        } else {
            let mut ctx = CTX.lock().unwrap();
            ctx.version_outdated = false;
        }
    }

    let resp = reqwest::blocking::Client::new()
        .post(format!("{HUB_URL}/api/is_node_online/{}", node_id))
        .json(&body)
        .send();

    if resp.is_err() {
        log(ERROR, "Hub unreachable");
    } else if !resp.as_ref().unwrap().status().is_success() {
        let err = resp.unwrap().text().unwrap();

        log(WARN, &format!("hub online response not OK: {}", err));
    } else if resp.as_ref().unwrap().status().is_success() {
        let mut nctx = NODE_CTX.lock().unwrap();

        let body = resp.unwrap().text().unwrap();

        if body == "true" {
            nctx.node_status = NodeStatus::Online;
        } else {
            nctx.node_status = NodeStatus::Offline;
        }
    }

    let total = get_hub_total_nodes();
    let online = get_hub_online_nodes();

    let mut nctx = NODE_CTX.lock().unwrap();

    if total.is_ok() {
        nctx.node_stats
            .total_nodes
            .push((timestamp, total.unwrap()));
    }

    if online.is_ok() {
        nctx.node_stats
            .online_nodes
            .push((timestamp, online.unwrap()));
    }

    let to_retain = 600;

    if nctx.node_stats.total_nodes.len() > to_retain {
        let length = nctx.node_stats.total_nodes.len();
        nctx.node_stats.total_nodes.drain(0..length - to_retain);
    }

    if nctx.node_stats.online_nodes.len() > to_retain {
        let length = nctx.node_stats.online_nodes.len();
        nctx.node_stats.online_nodes.drain(0..length - to_retain);
    }
}

fn start_node_hub_ping() {
    // let mut nctx = NODE_CTX.lock().unwrap();

    // let timestamp = get_timestamp_ms();

    // nctx.node_stats.total_nodes.push((timestamp, 1));

    // nctx.node_stats.online_nodes.push((timestamp, 1));

    // drop(nctx);

    ping_hub();

    loop {
        sleep_ms(HUB_PING_RATE);

        ping_hub();
    }
}

fn start_node_analytics() {
    loop {
        sleep_ms(1000);

        let timestamp = get_timestamp_ms();

        let storage_datum = (timestamp, get_storage_usage());

        let download_datum = (timestamp, get_download_bandwidth());
        let upload_datum = (timestamp, get_upload_bandwidth());

        let mut nctx = NODE_CTX.lock().unwrap();

        nctx.node_stats.storage_used.push(storage_datum);

        nctx.node_stats.download_bandwidth.push(download_datum);

        nctx.node_stats.upload_bandwidth.push(upload_datum);

        let to_retain = 120; // retain the last 120 seconds = 2 minutes

        if nctx.node_stats.storage_used.len() > to_retain {
            let length = nctx.node_stats.storage_used.len();
            nctx.node_stats.storage_used.drain(0..length - to_retain);
        }

        if nctx.node_stats.download_bandwidth.len() > to_retain {
            let length = nctx.node_stats.download_bandwidth.len();
            nctx.node_stats
                .download_bandwidth
                .drain(0..length - to_retain);
        }

        if nctx.node_stats.upload_bandwidth.len() > to_retain {
            let length = nctx.node_stats.upload_bandwidth.len();
            nctx.node_stats
                .upload_bandwidth
                .drain(0..length - to_retain);
        }
    }
}

fn restore_data() {
    let mut ctx = CTX.lock().unwrap();
    let mut nctx = NODE_CTX.lock().unwrap();

    let save_path = format!("{}/save.sdb", ctx.runtime_path);

    if utils_file_exists(&save_path) {
        let save_str = std::fs::read_to_string(save_path).unwrap();

        let state: SaveState = serde_json::from_str(&save_str).unwrap();

        nctx.resources = state.resources;
        nctx.node_status = state.node_status;
        nctx.node_stats = state.node_stats;

        ctx.config = state.config;
        ctx.chat = state.chat;
        ctx.settings = state.settings;

        for resource in &mut nctx.resources {
            resource.status = ResourceStatus::Pending;
        }

        ctx.chat.mount_status = ChatMountStatus::Unmounted;
    }
}

fn start_node() -> std::thread::JoinHandle<()> {
    let handler = std::thread::Builder::new()
        .name("start_node_service".to_string())
        .spawn(move || {
            start_node_service();
        })
        .unwrap();

    std::thread::Builder::new()
        .name("start_node_analytics".to_string())
        .spawn(move || {
            start_node_analytics();
        })
        .unwrap();

    std::thread::Builder::new()
        .name("start_node_hub_ping".to_string())
        .spawn(move || {
            start_node_hub_ping();
        })
        .unwrap();

    return handler;
}

pub fn handle_session(args: Args) -> Result<u64, String> {
    if args.command.as_ref().unwrap() == "run" {
        init_shog(args, true)?;

        restore_data();

        let server_handler = start_server();
        start_node();

        std::thread::Builder::new()
            .name("data_saver".to_string())
            .spawn(move || {
                data_saver();
            })
            .unwrap();

        server_handler.join().unwrap();
    } else {
        return Err(format!(
            "invalid command: {}",
            args.command.as_ref().unwrap()
        ));
    }

    return Ok(0);
}

#[derive(PartialEq)]
pub enum LogType {
    INFO,
    WARN,
    ERROR,
}

pub fn log(log_type: LogType, log: &str) {
    if log_type == INFO {
        let prefix = "[INFO]".bright_green();

        println!("{} {}", prefix, log);
    } else if log_type == WARN {
        let prefix = "[WARN]".bright_yellow();

        println!("{} {}", prefix, log);
    } else if log_type == ERROR {
        let prefix = "[ERROR]".bright_red();

        println!("{} {}", prefix, log);
    }
}
