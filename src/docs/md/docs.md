{{> head}}

<div id="top"></div>

# Shoggoth - Documentation

{{> table_of_contents}}

<div id="what-is-shoggoth"></div>

## What is Shoggoth?

Shoggoth is a peer-to-peer, anonymous network for publishing and distributing open-source Artificial Intelligence (AI).
To join the Shoggoth network, there is no registration or approval process.
Nodes operate anonymously with identifiers decoupled from real-world identities.
Anyone can freely join the network and immediately begin publishing or accessing resources.

The purpose of Shoggoth is to combat AI censorship and empower software developers to create and distribute open-source AI, without a centralized service or platform.
Shoggoth is developed and maintained by [ShogAI](https://shog.ai).

### ⚠️Disclaimer⚠️

Shoggoth is still in its beta stage. There is no guarantee that it will function as intended, and security vulnerabilities are likely to be present.
Shoggoth is rapidly evolving and all features/APIs are subject to change. Shoggoth should be considered experimental software.

Shoggoth comes with absolutely NO WARRANTY of any kind. Please email reports of any bugs/issues to netrunner@shog.ai

This documentation is a work in progress. It is not complete and may contain outdated information as Shoggoth is rapidly evolving.

Please be aware that Shoggoth is a public network! This means anything uploaded will be accessible by anybody on the network.

<div id="installation"></div>

## Installation

### Supported Platforms

Shoggoth currently supports only Linux and macOS operating systems. Windows support is still in development.

<div id="download-precompiled-binaries"></div>

### Download Precompiled Binaries

You can download Shoggoth from [https://shog.ai/download.html](https://shog.ai/download.html).
Once the download is complete, verify the checksum with the following command to ensure it was not tampered with in transit:

```bash
sha256sum shoggoth-v0.2.1-Linux-x86_64.zip
```

Ensure that the printed checksum is the same as the one displayed on the download page.

Extract the zip archive into your home directory:

```bash
unzip -o -q shoggoth-v0.2.1-Linux-x86_64.zip -d $HOME/shoggoth/
```

Your home directory should now contain a `shoggoth` directory. Shoggoth uses this directory as the runtime where essential data is stored.
Do not delete this directory or move it elsewhere.

cd into the `shoggoth` directory:

```bash
cd $HOME/shoggoth
```

Run the install script:

```bash
./scripts/install.sh
```

Congrats! You have successfully installed Shoggoth. You can now use the `shog` command to run and manage a Shoggoth Node.
You can run the below command to verify that it was installed correctly:

```bash
shog --version
```

<div id="build-from-source"></div>

### Build from Source

Shoggoth is written in the C programming language. All you need to build it from source is a C compiler, GNU make and a few dependencies.
Follow the below instructions to obtain the source code and build Shoggoth:

NOTE: if you encounter any problems while building Shoggoth from source, you can [create an issue](https://github.com/shog-ai/shoggoth/issues) on GitHub, or join the [Discord community](https://discord.com/invite/AG3duN5yKP) and we will be glad to assist you.

<div id="download-the-source-code"></div>

#### Download the Source Code

You can obtain the Shoggoth source code either by cloning the git repository or by downloading it as a ZIP archive from the ShogAI website.

Choose one of the following options to download the source code:

Using git:

```bash
git clone -b 0.2.1 https://github.com/shog-ai/shoggoth --depth 1 shoggoth-source
```

Or download the source code:

```bash
wget https://shog.ai/download/v0.2.1/shoggoth-source-v0.2.1.zip
```

```bash
  unzip -o -q ./shoggoth-source-v0.2.1.zip -d ./shoggoth-source
```

<div id="build-with-make"></div>

#### Build with Make

After downloading the source code, change into the directory that was downloaded:

```bash
cd shoggoth-source
```

Run `make` to build the code:

```bash
make
```

The `make` command may take a few seconds to finish. When it is done, you can then install Shoggoth with the below command:

```bash
make install
```

Congrats! You have successfully installed Shoggoth. You can now use the `shog` command to run and manage a Shoggoth Node.
You can run the below command to verify that it was installed correctly:

```bash
shog --version
```

<div id="concepts"></div>

## Concepts

<div id="shoggoth-resources"></div>

### Shoggoth Resources

The purpose of Shoggoth is to distribute Shoggoth Resources. A Shoggoth Resource is a single file that can be an ML model, code repository, research paper, dataset, or any other kind of file.

Shoggoth Resources are stored on Shoggoth nodes, and can only be published by Shoggoth nodes.

Shoggoth Resources are identified by their `Shoggoth ID`, which is their SHA256 Hash with a SHOG prefix, for example: 

```
SHOG437ebfbe1034295ba62d801fd5811ae1d355798746c8a042689d1b71f4471964
```

Hence, a Shoggoth ID is 68 characters long.

<div id="shoggoth-nodes"></div>

### Shoggoth Nodes

A Shoggoth node is a software program running on a computer. This software adheres to the Shoggoth protocol and communicates with other nodes in a peer-to-peer network.
The other nodes that a node communicates with are called its `peers`.

Shoggoth Nodes are capable of publishing new resources, and serving existing resources to their peers.

Nodes also provide an HTTP API that allows anyone to download resources. Hence, anyone can download resources from a Shoggoth node using a web browser.

Every node has a unique identifier called a Node ID that can be used to distinguish one node from another.

A Node ID is a 37 characters long string that looks like this:

```
SHOGNed21b1a13c07a5cba894bb9326d72133
```

If you installed Shoggoth from [the instructions above](#installation), then you already have a Shoggoth node, but it is not running yet.
To run the node, use the following command:

```bash
shog run
```

<div id="resource-pinning"></div>

### Resource Pinning

When a Shoggoth node stores a Shoggoth resource on its local storage, we say that the node has pinned the resource.
On the Shoggoth network, not all nodes locally store (pin) all resources.

Generally speaking, a particular resource will be stored on one or more nodes, but not all nodes.
Only nodes that have a local copy of a resource are said to have pinned the resource.

However, nodes that do not pin a specific resource can still be used to access it. Nodes simply forward requests that try to access unpinned resources to their peers that have pinned them.


<div id="public-and-private-keys"></div>

### Public and Private Keys

When you start a Shoggoth Node for the first time, an RSA key-pair is generated and stored in `$HOME/shoggoth/node/keys/`.
This directory will contain two files: public.txt and private.txt.
The public key in public.txt is an RSA public key that is publicly shared on the network and can be seen by everyone.
It will be shared among nodes when they communicate with one another.
However, the private key in private.txt is a secret and must be kept securely. Anyone who has access to the private key of a node controls its identity on the network.

The Node ID of a node is derived from a cryptographic hash of its public key.

<div id="how-to-use-shoggoth"></div>

## How to use Shoggoth

### Download a resource

screenshot

### Create a new resource

```bash
shog pin <file> <label>
```

### Clone a resource

```bash
shog clone <url>
```

### Command Line Flags

The `shog` command accepts a few command line flags that can be used to customize its behavior.
To see a list of all the available flags, run the below command:

```bash
shog -h
```


<div id="how-does-shoggoth-work"></div>

## How does Shoggoth Work?

<div id="nodes"></div>

### Nodes

Every Shoggoth node stores and maintains a local data structure called a Distributed Hash Table (DHT). This DHT is stored in an in-memory database.
The DHT of a node is a table that contains information about all its peers, with each item in the table representing information about a peer, including its Node ID, Public Key, IP address or domain name,
and the Shoggoth IDs of all resources it has pinned.

This DHT will be updated frequently as the node discovers new peers, and the list of pinned resources for all peers will also be updated periodically.

Nodes frequently exchange DHTs in such a way that a new node can instantly become aware of all other nodes and their pins by simply collecting the DHT of a node that is part of the network and merging it with its local DHT.

This is the backbone of the Shoggoth Network. By its design, every node is able to communicate with every other node directly, without relays, by simply querying its local DHT for the IP address or domain name of the desired node, and sending HTTP requests to it.

Shoggoth Nodes communicate by sending and receiving HTTP requests according to the [Shoggoth Node API](#api). With this API, nodes can obtain the DHTs of their peers and also download Shoggoth Resources from their peers to pin them locally.

When a new node joins the network, it shares a manifest containing its public key, Node ID, and IP address or domain name to a set of known nodes.
Simply put, a new node needs to know at least one already existing node when joining the network. This already existing node will add the new node to its DHT, thereby introducing it to the rest of the network,
and also give the new node a copy of its DHT, thereby allowing the new node to independently access the rest of the network.

This already existing node that a new node uses to join the network is also called a `bootstrap node`. Any node can be used as a bootstrap node.

Once the new node obtains the DHT of the bootstrap node, it will add all the peers of the bootstrap node to its local DHT and also request for the DHTs of its new peers to increase its network reach.

There are many public nodes available that can be used as bootstrap nodes. One of them is [https://shoggoth.network](https://shoggoth.network) which is run and maintained by ShogAI.
This is the default bootstrap node which can easily be changed in a [configuration file](#configuration). If you know someone who already runs a node, you can use theirs as a bootstrap node. All you need is the public address of the node.
This public address can be its domain name like `http://shoggoth.network`, only an IP address (assuming port 80) like `http://8.8.8.8`, or an IP address with a specified port like `http://8.8.8.8:6969`

Every Shoggoth node exposes an HTTP API which it uses to receive requests from other nodes and clients. It is this API that underpins the protocol that nodes use to exchange DHTs, fetch resources, and perform other network activities.
The API is documented [here](/explorer/docs/api).

<div id="persistence"></div>

### Persistence

The pinning of resources is voluntary. Therefore a node can decide to pin any resource on the network or unpin (delete) a resource from its storage. There is no guarantee that at least one node will pin a resource. Therefore the best way to guarantee that a resource will remain pinned is to set up your own node.

<div id="configuration"></div>

## Configuration

Shoggoth nodes are configured with a config.toml file located in $HOME/shoggoth/node/

<div id="api"></div>

## Node HTTP API

All Shoggoth nodes expose an HTTP API that peers and clients can use to interact with them. This API is rate-limited to avoid spam and DOS attacks.
The API is documented [here](/explorer/docs/api).

<div id="explorer"></div>

## Shoggoth Explorer

Shoggoth Explorer is a web interface exposed by all Shoggoth Nodes, which can be used to lookup and inspect Shoggoth Nodes and Resources.
Shoggoth Explorer also contains documentation for Shoggoth and a reference for the Shoggoth Node API.

The documentation you are reading is part of the Shoggoth Explorer. If you have installed Shoggoth, you also have a copy of the Shoggoth Explorer on your computer.
You can access it by running a Shoggoth node:

```bash
  shog run
```

The above command will start a Shoggoth node and expose the Shoggoth explorer at http://127.0.0.1/explorer

<div id="exposing-a-shoggoth-node-to-the-internet"></div>

## Exposing a Shoggoth Node to the Internet

⚠️WARNING⚠️

Before you expose a node to the internet, be aware that Shoggoth is still experimental software.
Thus, there may be security vulnerabilities in the code.
To minimize the harm caused by such vulnerabilities, we recommend running a Shoggoth node in an isolated environment.
Ideally in a cloud or local virtual machine.

While Shoggoth is still experimental, assume that the computer running the node will be vulnerable to attacks.
Therefore, we recommend that you do not expose a Shoggoth node to the internet if it is running on your personal computer or on a computer that runs critical software for other purposes.
We recommend using a dedicated machine whose purpose is only to serve as a Shoggoth Node, with no other critical applications or data on it.

For this reason, the default configuration that comes with a fresh Shoggoth installation does not expose the node to the internet.

Below are the necessary changes you need to make to expose your node to the internet.

### Configuration

To take your node public, you need to make a couple of modifications to your node config in `$HOME/shoggoth/node/config.toml`

#### 1. Change the Host and Port

1.1 Change the `host` field in the `network` table from "127.0.0.1" to "0.0.0.0".

1.2 Change the `port` field to whatever port you wish.
You can leave it as the default `6969` or change it to `80` if you intend to use a domain name or depending on your server setup.

```toml
  [network]
  host = "0.0.0.0"
  port = 6969
```

#### 2. Change Public Host

Change the `public_host` field in the `network` table to the public IP address of your computer (the IP address that can be used to reach you from the internet, not your private IP address), or domain name that points to your computer.
If using an IP address, indicate the port after the IP address like `8.8.8.8:6969`. The port you indicate must be the same as the one you set in the `port` field above. If you are using a reverse proxy, then use the port that the reverse proxy exposes.
If you don't indicate a port, the default port `80` will be used. If using a domain name, port `80` will be used.

Ensure to add a "http://" or "https://" prefix depending on whether you have set up SSL. If you have not set up SSL, use "http://".

The below example configuration illustrates using an IP address `8.8.8.8` with a port `6969`. Note that the port should not be blocked by your firewall.

```toml
  [network]
  host = "0.0.0.0"
  port = 6969
  public_host = "http://8.8.8.8:6969"
```

The below example configuration illustrates using a domain name `shoggoth.network`, with a reverse proxy such as nginx redirecting traffic from port 80 to the node service running at port 6969:

```toml
  [network]
  host = "0.0.0.0"
  port = 6969
  public_host = "http://shoggoth.network"
```

<div id="system-requirements"></div>

### System Requirements

A Shoggoth node is very light on system resources and barely uses any compute while idle.
A machine with at least 1GB of RAM and 25GB of storage can be considered the bare minimum required to reliably run a node.

<div id="contributing"></div>

## Contributing to Shoggoth

The Shoggoth project is currently being developed on [Github](https://github.com/shog-ai).

Before making pull requests, first create a new issue on the [issues page](https://github.com/shog-ai/shoggoth/issues) outlining the problems you want to solve, the feature you want to add, or the bug you want to fix.

There are dependencies in `./lib/`. Some of these dependencies are external dependencies, meaning they are not part of the Shoggoth project.
To contribute to external dependencies, please refer to their respective GitHub repositories listed in [dependencies](#dependencies).

Some other dependencies including sonic, camel, tuwi, and netlibc are internal dependencies, meaning they are developed alongside Shoggoth.
You can contibute to them by making your changes within the Shoggoth repository itself, and prefixing your commits and PRs with the name of the dependency like "sonic: fixed a sonic specific bug".

### Coding Guidelines

- Avoid adding third-party dependencies, extra files, extra headers, etc.
- Always consider cross-compatibility with other operating systems and architectures.
- Spend a little time reading the existing code to get a feel for the style. Try to follow the patterns in the code.

You can join the [Discord community](https://discord.com/invite/AG3duN5yKP) to ask questions concerning the coding style, the code architecture, and whatever problems you encounter.

<div id="faq"></div>

## Frequently Asked Questions (FAQ)

Q: Is Shoggoth free to use?
<br />
A: Yes, Shoggoth is 100% free and open source software. There are no fees or licenses required to access the network as a user or run a node.

Q: How does Shoggoth prevent censorship?
<br />
A: Shoggoth uses a decentralized peer-to-peer network to make censorship virtually impossible. Content is replicated across many volunteer nodes instead of centralized servers.

Q: Can Shoggoth be taken down or blocked?
<br />
A: It is extremely unlikely that Shoggoth can be completely blocked since there is no central point of failure to attack. Nodes are run independently all over the world. As long as some nodes remain active, the network persists.

Q: Who created Shoggoth?
<br />
A: Shoggoth was created by Netrunner KD6-3.7 (email netrunner@shog.ai) at [ShogAI](https://shog.ai).

Q: How is the development of Shoggoth funded?
<br />
A: The development of Shoggoth is funded by [donations and sponsorships](#donate).

Q: How can I get involved with the Shoggoth community?
<br />
A: You can join the [Discord community](https://discord.com/invite/AG3duN5yKP), follow [Shoggoth on Twitter](https://twitter.com/shog_AGI) and contribute to the open source code on [GitHub](https://github.com/shog-ai).

Q: How fast is data transfer on the Shoggoth network?
<br />
A: Speeds depend on node capacity and user demand, but are generally quite fast.

Q: Is there a limit on how much I can publish on Shoggoth?
<br />
A: There is no limit

Q: How can I donate to support Shoggoth's development?
<br />
A: You can donate online with a credit card at [shog.ai/donate.html](https://shog.ai/donate.html), send Bitcoin or Ethereum to the addresses listed at [shog.ai/donate.html](https://shog.ai/donate.html). Corporate sponsorships are also available (email netrunner@shog.ai).

Q: Does Shoggoth use any blockchain technologies?
<br />
A: No. Shoggoth is not a blockchain and currently does not use any blockchain technologies or cryptocurrencies.

## Technical Details

### Node Auto-Update

By default, Shoggoth Nodes will check for updates on the Shoggoth network and download them if found.
This means your node can receive the latest features, security updates, and bug fixes without your intervention.
You can turn off auto-update in the node configuration:

```toml
  [update]
  enable = false
```

<div id="dependencies"></div>

### Dependencies

All external dependencies for the Shoggoth project are housed within the Shoggoth repository itself.
They can be found in the `./lib` directory in the source code.

Some of the dependencies were developed by ShogAI specifically for the Shoggoth project, while others are obtained from external open-source projects unaffiliated with ShogAI or the Shoggoth Project.

Some of the dependencies developed by ShogAI include:

#### Sonic

Sonic is an HTTP server and client library for the C programming language. Sonic is the library used for implementing the Shoggoth Node API and the Shoggoth Client.
You can read the [Sonic documentation](/explorer/docs/sonic) for more information.

#### Camel

Camel is a testing framework for the C programming language. It can be used for writing unit tests, integration tests, functional tests, and fuzz tests. Camel is the framework used for testing Shoggoth, Sonic, and Tuwi.
You can read the [Camel documentation](/explorer/docs/camel) for more information.

#### Tuwi

Tuwi is a terminal user interface framework for the C programming language. Tuwi is the framework used for implementing the terminal UI of Shoggoth and Camel.
You can read the [Tuwi documentation](/explorer/docs/tuwi) for more information.

#### Other Dependencies

The below dependencies are used in the Shoggoth project, sourced from external open-source projects:

- [cjson](https://github.com/DaveGamble/cJSON)
- [tomlc](https://github.com/cktan/tomlc99)
- [md4c](https://github.com/mity/md4c/)

#### Required System Utilities

The below commands are required to be installed on a computer in order to run a Shoggoth Node.
All these commands are usually already pre-installed on macOS and Linux operating systems, so they don't have to be installed again.

- git
- GNU tar (not the default tar on macOS. install it on macOS using homebrew "brew install gnu-tar")
- cat
- sha256sum
- addr2line
- find
- sort

## More Information

<div id="donate"></div>

### Donate/Sponsor Shoggoth

You can donate with a credit card at [shog.ai/donate.html](https://shog.ai/donate.html), send Bitcoin or Ethereum to the addresses listed at [shog.ai/donate.html](https://shog.ai/donate.html).
Corporate sponsorships are also available (email netrunner@shog.ai).

### Get Help/Support

If you encounter any bugs or need assistance with anything, do not hesitate to join the [Discord community](https://discord.com/invite/AG3duN5yKP). You can also email netrunner@shog.ai.

### License

Shoggoth is licensed under the MIT license. Read the LICENSE file in the source code for more information

Shoggoth uses a few dependencies which have their own licences. The dependencies in the ./lib/ directory of the Shoggoth source code are independent of the MIT license used for Shoggoth.
The source code for each dependecy includes a LICENSE file indicating the license that covers it.

### Community

Join the [Discord community](https://discord.com/invite/AG3duN5yKP).

### Popular nodes

Some popular nodes include:

[https://shoggoth.network](https://shoggoth.network)

[https://node1.shog.ai](https://node1.shog.ai)

[https://node2.shog.ai](https://node2.shog.ai)

{{> end}}
