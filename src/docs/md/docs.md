{{> head}}

# Shoggoth - Documentation

{{> table_of_contents}}

<div id="what-is-shoggoth"></div>

## What is Shoggoth?

Shoggoth is a peer-to-peer, anonymous network for publishing and distributing open-source code, Machine Learning models, datasets, and research papers.
To join the Shoggoth network, there is no registration or approval process.
Nodes and clients operate anonymously with identifiers decoupled from real-world identities.
Anyone can freely join the network and immediately begin publishing or accessing resources.

The purpose of Shoggoth is to combat software censorship and empower software developers to create and distribute software, without a centralized hosting service or platform.
Shoggoth is developed and maintained by [Shoggoth Systems](https://shoggoth.systems), and its development is funded by [donations and sponsorships](#donate).

### ⚠️Disclaimer⚠️

Shoggoth is still in its beta stage. There is no guarantee that it will function as intended, and security vulnerabilities are likely to be present.
Shoggoth is rapidly evolving and all features/APIs are subject to change. Shoggoth should be considered experimental software.

Shoggoth comes with absolutely NO WARRANTY of any kind. Please email reports of any bugs/issues to netrunner@shoggoth.systems

This documentation is a work in progress. It is not complete and may contain outdated information as Shoggoth is rapidly evolving.

Please be aware that Shoggoth is a public network! This means anything uploaded will be accessible by anybody on the network.

<div id="installation"></div>

## Installation

### Supported Platforms

Shoggoth currently supports only Linux and macOS operating systems. Windows support is still in development.

<div id="download-precompiled-binaries"></div>

### Download Precompiled Binaries

You can download Shoggoth from [shoggoth.systems/download.html](https://shoggoth.systems/download.html).
Once the download is complete, verify the checksum with the following command to ensure it was not tampered with in transit:

```bash
sha256sum shoggoth-v0.1.5-Linux-x86_64.zip
```

Ensure that the printed checksum is the same as the one displayed on the download page.

Extract the zip archive into your home directory:

```bash
unzip -o -q shoggoth-v0.1.5-Linux-x86_64.zip -d $HOME/shoggoth/
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

Congrats! You have successfully installed Shoggoth. You can now use the `shog` command as a Shoggoth Client and as a Shoggoth Node (server).
You can run the below command to verify that it was installed correctly:

```bash
shog --version
```

<div id="build-from-source"></div>

### Build from Source

You can build Shoggoth from source. Since Shoggoth is written in the C programming language, all you need to build it is a C compiler and GNU make.
Follow the below instructions to obtain the source code and build Shoggoth:

NOTE: if you encounter any problems while building Shoggoth from source, you can [create an issue](https://github.com/shoggoth-systems/shoggoth/issues) on GitHub, or join the [Discord community](https://discord.com/invite/AG3duN5yKP) and we will be glad to assist you.

<div id="download-the-source-code"></div>

#### Download the Source Code

You can obtain the Shoggoth source code either by using git to clone the repository or by downloading it as a Zip archive from the Shoggoth Systems website.

Choose one of the following options to download the source code:

Using git:

```bash
git clone -b 0.1.5 https://github.com/shoggoth-systems/shoggoth shoggoth-source
```

Or download the source code:

```bash
wget https://shoggoth.systems/download/v0.1.5/shoggoth-source-v0.1.5.zip
```

Then extract the downloaded zip:

```bash
  unzip -o -q ./shoggoth-source-v0.1.5.zip -d ./shoggoth-source
```

<div id="build-with-make"></div>

#### Build with Make

After downloading the source either by using git or from the download link, change into the directory that was downloaded:

```bash
cd shoggoth-source
```

Run `make` to build the code:

```bash
make
```

The `make build` command may take a few minutes to finish. When it is done, you can then install Shoggoth with the below command:

```bash
make install
```

Congrats! You have successfully installed Shoggoth. You can now use the `shog` command as a Shoggoth Client and as a Shoggoth Node (server).
You can run the below command to verify that it was installed correctly:

```bash
shog --version
```

<div id="concepts"></div>

## Concepts

<div id="shoggoth-resources"></div>

### Shoggoth Resources

On the Shoggoth network, data such as code repositories, research papers, Machine Learning models, and datasets are called resources,
and these resources are stored on servers called nodes.

<div id="shoggoth-nodes"></div>

### Shoggoth Nodes

A Shoggoth node is a software program running on a computer. This software adheres to the Shoggoth protocol and communicates with other nodes in a peer-to-peer network.
The other nodes that a node communicates with are called its `peers`.

A Shoggoth node can also be used by a Shoggoth client as a gateway to publish or download resources on the network.

Every node has a unique identifier called a Node ID that can be used to distinguish one node from another.

A Node ID is a 37 characters long string that looks like this:

```
SHOGNed21b1a13c07a5cba894bb9326d72133
```

If you installed Shoggoth from [the instructions above](#installation), then you already have a Shoggoth node, but it is not running yet.
To run the node, use the following command:

```bash
shog node run
```

<div id="shoggoth-clients"></div>

### Shoggoth Clients

A Shoggoth client is a software program that is capable of publishing or downloading resources on the Shoggoth network.
It does so by sending HTTP requests to Shoggoth Nodes.

If you installed Shoggoth from [the instructions above](#installation), then you already have a Shoggoth client.

Every Shoggoth Client has a unique identifier called a Shoggoth ID that can be used to distinguish one client from another.

A Shoggoth ID is a 36 characters long string that looks like this:

```
SHOGed21b1a13c07a5cba894bb9326d72133
```

The following is an example of using the Shoggoth client to download a code repository from a profile:

```bash
shog client clone SHOGed21b1a13c07a5cba894bb9326d72133/code/myrepo
```

To differentiate a Shoggoth ID from a Node ID, check the prefix of the ID. A Node ID has a `SHOGN` prefix and a Shoggoth ID has a `SHOG` prefix.
Consequently, a Node ID is 37 characters long and a Shoggoth ID is 36 characters long.

The Shoggoth ID of a client is its unique address on the Shoggoth network and it points to the client's Shoggoth Profile.

<div id="shoggoth-profile"></div>

### Shoggoth Profile

A Shoggoth Profile is just like any other online profile. It is a mechanism by which the network groups resources by publishers.
Every Shoggoth Client controls a single Shoggoth Profile. This profile contains all the resources published by the client.

Imagine your Shoggoth profile like your GitHub profile. A Github profile is identified with a username and controlled by a password.
Likewise, a Shoggoth profile is identified with a Shoggoth ID and controlled by an [RSA private key](#public-and-private-keys).

You can also think of a profile as a folder. This folder then contains 4 sub-folders (code, models, papers, datasets) for organizing their resources.
The code folder will further contain a number of git repositories, the papers folder will contain a number of PDF files, the models folder will contain a number of ML models, and so on.

You can view a profile's resources in a web browser by attaching its Shoggoth ID to the URL of a node.
All Shoggoth nodes provide a public webpage called a [Shoggoth Explorer](#explorer) that can be used to lookup any Shoggoth ID.
Assuming there is a node at the URL `https://shoggoth.network`,
you can view the profile of a Shoggoth ID `SHOGed21b1a13c07a5cba894bb9326d72133` by attaching it to the URL like `https://shoggoth.network/explorer/profile/SHOGed21b1a13c07a5cba894bb9326d72133`

You can further explore a group of resources such as code repositories by attaching it to the URL like:

Code Repositories

```text
https://shoggoth.network/explorer/profile/SHOGed21b1a13c07a5cba894bb9326d72133/code
```

ML Models

```text
https://shoggoth.network/explorer/profile/SHOGed21b1a13c07a5cba894bb9326d72133/models
```

Datasets

```text
https://shoggoth.network/explorer/profile/SHOGed21b1a13c07a5cba894bb9326d72133/datasets
```

Research Papers

```text
https://shoggoth.network/explorer/profile/SHOGed21b1a13c07a5cba894bb9326d72133/papers
```

Furthermore, you can navigate to a specific resource such as a specific code repository:

```text
https://shoggoth.network/explorer/profile/SHOGed21b1a13c07a5cba894bb9326d72133/code/myrepo
```

These links can be shared on websites, social media, and anywhere else, as they are simply URLs pointing to a profile or resource.

On the client's computer, a profile is simply a directory. This directory contains 4 sub-directories (code, models, datasets, and papers), and each subdirectory contains one or more resources:

```text
shoggoth-profile
- code
  - code1
  - code2
- models
  - model1
  - model2
- datasets
  - dataset1
  - dataset2
- papers
  - paper1
  - paper2
  - paper3
```

Every resource is simply a git repository that contains the necessary files.
For example, a `papers` resource is a git repository that may contain one or more PDF files or LaTex files.
Likewise, a `models` resource is a git repository that may contain the ML model in whatever file format is desired, for example, .pb, .onnx, .mlmodel, .pt, .pmml, .xml, .zip, .csv, and so on.

There are no rules or restrictions on what file formats can be used in resources.
Since Shoggoth resources are simply git repositories, you can store any file formats in them and any number of files or sub-directories can be within a resource.

Every valid git repository is a valid Shoggoth resource and every valid Shoggoth resource is a valid git repository.
An ideal Shoggoth resource will contain a README file with general information about the resource and instructions on how to use it.
It should also contain a LICENSE file and other supporting files, however, these are not rules but only conventions.
A resource could also be empty with no files and still be a valid resource.

By default, nodes limit profiles to a maximum size of 50 MB. However, individual nodes can increase or decrease this limit.
To publish large resources, such as resources as large as 100 Gigabytes, you may have to set up a node where you pin only your own profile.
On a personal Shoggoth node, you have the freedom to pin Gigabytes (and possibly even terabytes) of resources in a single profile, and Shoggoth clients will be able to download them from your node, or any node on the network that can reach your node.

<div id="profile-pinning"></div>

### Profile Pinning

When a node stores a profile on its local storage, we say that the node has pinned the profile.
On the Shoggoth network, not all nodes locally store all profiles.
Generally speaking, a particular profile will be stored on many nodes, but not all nodes. Only nodes that have a local copy of a profile are said to have pinned the profile.
However, nodes that have not pinned a profile can still be used to access them.
Nodes simply forward requests that try to access a profile to their peers that have pinned it, or respond with an error if they have no peers that have pinned it.

When a client contacts a node, requesting to download a resource from a profile, the node may either have that profile pinned or know another node that has it pinned.
In the latter case, it simply forwards the request to the other node.

<div id="public-and-private-keys"></div>

### Public and Private Keys

On the Shoggoth network, profiles are mutable. This means you have the freedom of publishing resources to your profile, deleting resources, and modifying resources, changing their content.
However, only the original publisher of a profile can modify it, or add resources to it. This is enforced via cryptographic signatures.

When you start a Shoggoth client or Shoggoth Node for the first time, an RSA key-pair is generated and stored in `$HOME/shoggoth/client/keys/` for clients and `$HOME/shoggoth/node/keys/` for nodes.
This directory will contain two files: public.txt and private.txt.
The public key in public.txt is an RSA public key that is publicly shared on the network and can be seen by everyone.
It will be shared among nodes and clients when they communicate with one another.
However, the private key in private.txt is a secret and must be kept securely. Anyone who has access to your private key can control your Shoggoth profile FOREVER.
Think of your private key as a password that you cannot change. The only solution to a compromised private key is creating a new profile with a new key pair.

The Shoggoth ID of a client is derived from a cryptographic hash of its public key. Likewise, the Node ID of a node is derived from a cryptographic hash of its public key

When you perform an action such as publishing a profile, your client will use your private key to sign that action.
Nodes will then use your public key to verify that the action was performed by you.

Modifying a profile is achieved by publishing a new version of your profile with all the desired changes.
When a node receives a new version of an existing profile and verifies that it was signed by the same publisher, it simply replaces the old one with the new one.

For example, if you change some code in a code repository, you have to republish your entire profile.
Nodes that pin that profile will then replace their copy of the profile with the new one.

<div id="#how-to-use-shoggoth"></div>

## How to use Shoggoth - Overview

Once you have installed Shoggoth, you can use the `shog` command to perform various actions such as running a Shoggoth node or downloading/publishing resources as a client.
Here is an overview of how to use the `shog` command. A more detailed step-by-step guide is available in [the next section](#using-shoggoth-step-by-step).

NOTE: if you encounter any problems while following the below instructions, you can [create an issue](https://github.com/shoggoth-systems/shoggoth/issues) on GitHub.
If you need some assistance or have questions, join the [Discord community](https://discord.com/invite/AG3duN5yKP) and we will be glad to assist you.

The below command uses the Shoggoth client to clone a code repository:

```bash
shog client clone SHOGed21b1a13c07a5cba894bb9326d72133/code/myrepo
```

The below command starts a Shoggoth node:

```bash
shog node run
```

<div id="#creating-and-publishing-a-profile"></div>

### Creating and Publishing a Profile

As described earlier, nodes do not store individual resources but instead, they store whole profiles with all their resources included.
Therefore, to publish a resource, you have to publish your entire profile.

#### Creating a Profile

Use the below command to create a new Shoggoth profile in your current working directory:

```bash
shog client init
```

The above command will create a new directory in your current working directory named `shoggoth-profile`.

You can create as many profiles as you like, but since you can have only one profile on the network, only your last published profile will be persistent on the network.
Therefore, you would want to create only one profile and place it in a suitable location.
This profile will then contain all your code repositories, ML models, Datasets, and research papers.

#### Publishing a Profile

Make sure you are in the directory of your profile:

```bash
cd shoggoth-profile
```

then run the below command to publish it:

```bash
shog client publish
```

<div id="#updating-modifying-a-profile"></div>

#### Updating/Modifying a Profile

To update or modify your profile, change the content of the `shoggoth-profile` folder on your local machine, then publish the profile again.
When a node receives your new profile, it simply replaces the old one with the new one. Since your profile is a folder, you can simply drag and drop or copy files into it, then run the publish command again to update your profile on the network.

<div id="#downloading-profiles-and-resources"></div>

### Downloading Profiles and Resources

You can download a whole profile, a group of resources from the profile such as code, datasets, etc, or a specific resource.

Download a profile:

```bash
shog client clone SHOGed21b1a13c07a5cba894bb9326d72133
```

Download all code repositories of a profile:

```bash
shog client clone SHOGed21b1a13c07a5cba894bb9326d72133/code
```

Download a specific code repository:

```bash
shog client clone SHOGed21b1a13c07a5cba894bb9326d72133/code/mycoderepo
```

### Command Line Flags

The `shog` command accepts a few command line flags that can be used to customize its behavior.
To see a list of all the available flags, run the below command:

```bash
shog -h
```

<div id="using-shoggoth-step-by-step"></div>

## Using Shoggoth - Step-by-Step Examples

NOTE: if you encounter any problems while following the below instructions, you can [create an issue](https://github.com/shoggoth-systems/shoggoth/issues) on GitHub.
If you need some assistance or have questions, join the [Discord community](https://discord.com/invite/AG3duN5yKP) and we will be glad to assist you.

<div id="publishing-your-profile-for-the-first-time"></div>

### Publishing your Profile for the First Time

After installing Shoggoth, the first thing you may want to do is publish your profile and then update it with a code repository to get a feel of the workflow.
Here is a step-by-step guide:

1. Choose a parent directory

Deciding where to put your profile is up to you. You may choose to put it in your Desktop directory, Home directory, or anywhere else.
In this example, we will put it in the Desktop directory.
This means our profile will be located at `$HOME/Desktop/shoggoth-profile`.

First, cd into the Desktop:

```bash
cd $HOME/Desktop/
```

2. Initialize your profile

The below command will create a new directory inside the current working directory named `shoggoth-profile`:

```bash
shog client init
```

3. cd into your profile

```bash
cd ./shoggoth-profile
```

You should notice that there are 4 sub-directories in the `shoggoth-profile` directory and one hidden `.shoggoth` sub-directory.
The hidden `.shoggoth` directory contains metadata about your profile.

4. Publish your profile

While inside the `shoggoth-profile` directory, run the below command:

```bash
shog client publish
```

This command packages your profile and sends it to a node. Make sure you are connected to the internet.

Once the publish command is done, you can visit your profile in a web browser by clicking the displayed link.

If you want to share your Shoggoth ID or save it somewhere, run the below command to display your Shoggoth ID:

```bash
shog client id
```

The output should look something like this:

```text
Your Shoggoth ID is: SHOGed21b1a13c07a5cba894bb9326d72133
```

Note that it may take a while for your profile to propagate across many nodes depending on the traffic on the network.
Ideally, your profile should reach most nodes within a couple of minutes.

Congrats! Your profile is now published. Follow the next guide to upload a code repository to your profile.

<div id="publishing-your-first-repository-to-your-profile"></div>

### Publishing your First Repository to your Profile

After publishing your profile, the next thing you may want to do is add a resource to your profile.
Here is a step-by-step guide to add a code resource to your profile:

1. cd into the code directory

In your profile directory, there are 4 main sub-directories. Among them, there is a `code` sub-directory.
This directory will contain all your code resources which are basically git repositories.

Assuming your profile is in your Desktop directory, cd into the `code` sub-directory:

```bash
cd $HOME/Desktop/shoggoth-profile/code
```

2. Create a new git repository:

```bash
git init ./myrepo
```

3. Create a python script

We don't want to leave our new code resource empty. In this example, we will write a single Python script that prints "Hello Shoggoth":

```bash
cd ./myrepo
```

```bash
echo "print(\"Hello Shoggoth\")" > ./hello.py
```

4. Add the file to git

You have to add the file to the git repository using `git add`, else it will not be included when uploading:

```bash
git add ./hello.py
```

5. To publish your changes, go back to the root of your Shoggoth profile:

```bash
cd ../../
```

6. Then run the publish command:

```bash
shog client publish
```

Congrats! You have published your first code repository! You can visit your profile in a web browser by clicking the displayed link to verify that your changes have propagated across the network.

This exact same process can be used to publish ML models, research papers, and datasets.
All you have to do is package them as git repositories by placing the necessary files in a git-initialized directory.

As long as at least one node pins your profile, a client from anywhere in the world can download it.

Read the next section to learn how to download resources.

<div id="downloading-a-profile"></div>

### Downloading a Profile

NOTE: When you want to download a resource or profile, you should not download it into your profile, else the next time you publish your profile, the downloaded resource will be published too.
Before downloading a profile or resource, ensure your current working directory is at a suitable location where you want the downloaded resource/profile to be saved.
It is not ideal to download resources/profiles into your own profile directory, so ensure to first cd into a different directory, for example, your Desktop:

```bash
cd $HOME/Desktop
```

Then download your desired profile using its Shoggoth ID:

```bash
shog client clone SHOGed21b1a13c07a5cba894bb9326d72133
```

To download a specific repository, attach the relative path to the end of the Shoggoth ID:

```bash
shog client clone SHOGed21b1a13c07a5cba894bb9326d72133/code/myrepo
```

After the download is complete, you should see a new folder in your current working directory, named after the Shoggoth ID if you downloaded a profile,
or the group name (like code, models, etc) if you downloaded a group of resources, or the name of the resource if you downloaded a single resource.

<div id="updating-your-repository"></div>

### Updating your Repository

After publishing the code repository to your profile, the next thing you may want to do is update it, since it is expected that software will be modified to newer versions.

Here is a step-by-step guide to update the `myrepo` repository we created earlier:

1. cd into the repository in your profile:

```bash
cd $HOME/Desktop/shoggoth-profile/code/myrepo
```

2. Change the content of the hello.py file to print "Hello world" instead of "Hello Shoggoth":

```bash
echo "print(\"Hello world\")" > ./hello.py
```

3. To publish your changes, go back to the root of your profile:

```bash
cd ../../
```

4. Run the publish command:

```bash
shog client publish
```

Now when you explore your profile by clicking the link printed by the publish command, you should notice that the content of the file has changed.

Note that it may take a few minutes or more for changes to your profile to propagate across all nodes that pin it.

<div id="how-does-shoggoth-work"></div>

## How does Shoggoth Work?

<div id="nodes"></div>

### Nodes

Every Shoggoth node stores and maintains a local data structure called a Distributed Hash Table (DHT). This DHT is stored in an in-memory database.
The DHT of a node is a table that contains information about all its peers, with each item in the table representing information about a peer, including its Node ID, Public Key, IP address or domain name,
and the Shoggoth IDs of all profiles it has pinned.

This DHT will be updated frequently as the node discovers new peers, and the list of pinned profiles for all peers will also be updated periodically.

Nodes frequently exchange DHTs in such a way that a new node can instantly become aware of all other nodes and their pins by simply collecting the DHT of a node that is part of the network and merging it with its local DHT.

This is the backbone of the Shoggoth Network. By its design, every node is able to communicate with every other node directly, without relays, by simply querying its local DHT for the IP address or domain name of the desired node, and sending HTTP requests to it.

Shoggoth Nodes communicate by sending and receiving HTTP requests according to the [Shoggoth Node API](#api). With this API, nodes can obtain the DHTs of their peers and also download Shoggoth Profiles from their peers to pin them locally.

When a new node joins the network, it shares a manifest containing its public key, Node ID, and IP address or domain name to a set of known nodes.
Simply put, a new node needs to know at least one already existing node when joining the network. This already existing node will add the new node to its DHT, thereby introducing it to the rest of the network,
and also give the new node a copy of its DHT, thereby allowing the new node to independently access the rest of the network.

This already existing node that a new node uses to join the network is also called a `bootstrap node`. Any node can be used as a bootstrap node.

Once the new node obtains the DHT of the bootstrap node, it will add all the peers of the bootstrap node to its local DHT and also request for the DHTs of its new peers to increase its network reach.

There are many public nodes available that can be used as bootstrap nodes. One of them is [https://shoggoth.network](https://shoggoth.network) which is run and maintained by Shoggoth Systems.
This is the default bootstrap node which can easily be changed in a [configuration file](#configuration). If you know someone who already runs a node, you can use theirs as a bootstrap node. All you need is the public address of the node.
This public address can be its domain name like `http://shoggoth.network`, only an IP address (assuming port 80) like `http://8.8.8.8`, or an IP address with a specified port like `http://8.8.8.8:6969`

Every Shoggoth node exposes an HTTP API which it uses to receive requests from other nodes and clients. It is this API that underpins the protocol that nodes use to exchange DHTs, fetch resources, and perform other network activities.
The API is documented [here](/explorer/docs/api).

<div id="clients"></div>

### Clients

Shoggoth clients access the network only through Shoggoth nodes. A client keeps a list of known nodes, which it updates occasionally. Whenever a client needs to perform an action such as publish a resource or download a resource, it randomly picks one of those nodes as a gateway to the network.

Shoggoth clients communicate with nodes via the same HTTP API used by nodes to communicate with one another.

<div id="persistence"></div>

### Persistence

The pinning of profiles is voluntary. Therefore a node can decide to pin any profile on the network or unpin (delete) a profile from its storage. There is no guarantee that at least one node will pin a profile. Therefore the best way to guarantee that a profile will remain pinned is to set up your own node.

By default, all nodes are configured to pin random profiles they come across. The network is designed to accommodate as many profiles as possible without the need for node maintainers to manually pin a profile.

Generally speaking, your profile should remain pinned for a very long time. If by any chance your profile gets unpinned by all nodes, you can simply republish it, and it will be as though it was never unpinned.

Sometimes, a node may run out of storage as it is a limited resource. In such a case, the node performs an action called "garbage collection" where it unpins (deletes) some profiles from itself to recover space. This process happens automatically.

Garbage collection affects entire profiles. This means as long as a node pins a profile, all its resources will remain on the node. If the garbage collector unpins the profile, all its resources will be deleted.

By default, a node only unpins the least active profiles.
Therefore if a profile has not been updated for a very long time, it is more likely to be removed by the garbage collector. This means popular/active profiles are more likely to remain persistent on the network, while stale and inactive profiles are more likely to be unpinned by nodes.

However, different nodes can have different garbage collection policies. Larger, more resourceful nodes have a higher threshold for garbage collection because they can accommodate more profiles due to abundant storage space.

Note that garbage collection is local to individual nodes. A node may unpin a profile while other nodes still have it pinned. Certain nodes can even turn off garbage collection completely.

You can decide to set up a Shoggoth node where you pin only your own profile or profiles of people you know and turn off garbage collection completely.

<div id="configuration"></div>

## Configuration

### Configuring a Node

Shoggoth nodes are configured with a config.toml file located in $HOME/shoggoth/node/

### Configuring a Client

Shoggoth clients are configured with a config.toml file located in $HOME/shoggoth/client/

<div id="api"></div>

## Node HTTP API

All Shoggoth nodes expose an HTTP API that peers and clients can use to interact with them. This API is rate-limited to avoid spam and DOS attacks.
The API is documented [here](/explorer/docs/api).

<div id="explorer"></div>

## Shoggoth Explorer

Shoggoth Explorer is a web interface exposed by all Shoggoth Nodes, which can be used to lookup Shoggoth Profiles and explore their resources.
Shoggoth Explorer also contains documentation for Shoggoth and a reference for the Shoggoth Node API.

The documentation you are reading is part of the Shoggoth Explorer. If you have installed Shoggoth, you also have a copy of the Shoggoth Explorer on your computer.
You can access it by running a Shoggoth node:

```bash
  shog node run
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

Note that your Internet Service Provider (ISP) may block incoming internet traffic if you are using a home router.

#### 3. Allow Publish, Enable Downloader

By default, your node will not accept requests to publish new profiles. Change the `allow_publish` field in the `pins` table to `true` to allow publishing.

Also, change the `enable_downloader` field to `true`, to allow your node to download new pins from its peers.

```toml
[pins]
allow_publish = true
enable_downloader = true
```

<div id="system-requirements"></div>

### System Requirements

A Shoggoth node is very light on system resources and barely uses any compute while idle.
A machine with at least 1GB of RAM and 25GB of storage can be considered the bare minimum required to reliably run a node.

<div id="contributing"></div>

## Contributing to Shoggoth

The Shoggoth project is currently being developed on [Github](https://github.com/shoggoth-systems).

Before making pull requests, first create a new issue on the [issues page](https://github.com/shoggoth-systems/shoggoth/issues) outlining the problems you want to solve, the feature you want to add, or the bug you want to fix.

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
A: Shoggoth was created by Netrunner KD6-3.7 (email netrunner@shoggoth.systems) at [Shoggoth Systems](https://shoggoth.systems).

Q: How is the development of Shoggoth funded?
<br />
A: The development of Shoggoth is funded by [donations and sponsorships](#donate).

Q: How can I get involved with the Shoggoth community?
<br />
A: You can join the [Discord community](https://discord.com/invite/AG3duN5yKP), follow [Shoggoth on Twitter](https://twitter.com/shoggothsystems) and contribute to the open source code on [GitHub](https://github.com/shoggoth-systems).

Q: How fast is data transfer on the Shoggoth network?
<br />
A: Speeds depend on node capacity and user demand, but are generally quite fast.

Q: Is there a limit on how much I can publish on Shoggoth?
<br />
A: By default, nodes impose soft limits of 50MB per profile, but you can run a personal node to remove limits.

Q: How can I donate to support Shoggoth's development?
<br />
A: You can donate online with a credit card at [shoggoth.systems/donate.html](https://shoggoth.systems/donate.html), send Bitcoin or Ethereum to the addresses listed at [shoggoth.systems/donate.html](https://shoggoth.systems/donate.html). Corporate sponsorships are also available (email netrunner@shoggoth.systems).

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

Some of the dependencies were developed by Shoggoth Systems specifically for the Shoggoth project, while others are obtained from external open-source projects unaffiliated with Shoggoth Systems or the Shoggoth Project.

Some of the dependencies developed by Shoggoth Systems include:

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
- [redis](https://github.com/redis/redis)
- [redisjson](https://github.com/RedisJSON/RedisJSON) (NOTE: RedisJSON's source code is open, but it has a proprietary license. We are working on replacing this dependency)
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

### Donate, Sponsor Shoggoth

You can donate with a credit card at [shoggoth.systems/donate.html](https://shoggoth.systems/donate.html), send Bitcoin or Ethereum to the addresses listed at [shoggoth.systems/donate.html](https://shoggoth.systems/donate.html).
Corporate sponsorships are also available (email netrunner@shoggoth.systems).

### Get Help/Support

If you encounter any bugs or need assistance with anything, do not hesitate to join the [Discord community](https://discord.com/invite/AG3duN5yKP). You can also email netrunner@shoggoth.systems.

### License

Shoggoth is licensed under the MIT license. Read the LICENSE file in the source code for more information

Shoggoth uses a few dependencies which have their own licences. The dependencies in the ./lib/ directory of the Shoggoth source code are independent of the MIT license used for Shoggoth.
The source code for each dependecy includes a LICENSE file indicating the license that covers it.

### Community

Join the [Discord community](https://discord.com/invite/AG3duN5yKP).

### Popular nodes

Some popular nodes include:

[https://shoggoth.network](https://shoggoth.network)

[https://node1.shoggoth.systems](https://node1.shoggoth.systems)

[https://node2.shoggoth.systems](https://node1.shoggoth.systems)

{{> end}}
