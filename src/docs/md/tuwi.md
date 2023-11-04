{{> head}}

# Tuwi - A terminal UI framework for C

## What is Tuwi?

Tuwi is a terminal user interface framework written in the C programming language, developed by [Shoggoth Systems](https://shoggoth.systems) for use in the Shoggoth project.

Tuwi is the framework used for implementing the terminal user interface of Shoggoth, Camel, and other Shoggoth projects.

### ⚠️Disclaimer⚠️
This library is still early in development and should be considered unstable and experimental.

This documentation is a work in progress. It is not complete and may contain invalid information as Tuwi is rapidly evolving.

## EXAMPLES

Examples can be found in /examples

You can run examples with:
```bash
$ E=<example_name> make run-example
```

where <example_name> is the name of the example you want to run. For example:

```bash
$ E=simple make run-example
```

## BUILDING TUWI

### Supported platforms

Tuwi currently supports only Linux and macOS operating systems.

### Requirements

* GNU Make


### Building

#### Clone the repository

```bash
$ git clone https://github.com/shoggoth-systems/tuwi
```

#### cd into the cloned directory

```bash
$ cd tuwi
```


#### Build with make

```
$ make build
```

The above command will build a static library into ./target/libtuwi.a which you can link with your project.

## USING TUWI

To use Tuwi, include the header ./tuwi.h in your project, then link with the static library in ./target/libtuwi.a

You can check the /examples directory for examples of how to use Tuwi, or read the documentation below

## CONTRIBUTING

Please follow the [Shoggoth contribution guidelines](/explorer/docs#contributing).

{{> end}}

