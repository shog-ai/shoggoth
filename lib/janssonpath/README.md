# Janssonpath
Jsonpath implementation for [jansson](http://www.digip.org/jansson/)([Github](https://github.com/akheron/jansson))

## How to build

`make` in outmost directory to get janssonpath.o.

Include janssonpath.h, and link with janssonpath.o and jansson in your project to use the facility.

Typical command for compile:

`cc your_sources janssonpath.o -ljansson -o target_name`

## Usage

`json_t * json_path_get(json_t *json, const char *path);`

json_path_get returns the result possiblely with reference to the json and its descendant nodes.

User is responsible for json_decref-ing the returned json node.

## Grammar

Use `"` to enclosing strings. Space characters not allowed except in string literal.

### Path

| Expression               |                                                                                     Description                                                                                     |
| ------------------------ | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------: |
| `$`                      |                                                             The root object or array. In out most path it can be omited.                                                             |
| `.property`              |                                                                               Same as `["property"]`.                                                                                |
| `["property"]`           |                                          Selects the specified property in a parent object. When property is integer n, it's alias of [n].                                           |
| `[n]`                    |                              Selects the n-th element from an array. Indexes are 0-based. Note that objects will always return empty lists for this.                              |
| `..`             |                 Recursive: all descendant node.                 |
| `*`                      |                                                Selects all elements in an object or an array, regardless of their names or indexes.                                                 |
| `#`                      |                                                      Length of the array.                                                       |
| `[start:end]` `[start:]` | Selects array elements from the start index and up to, but not including, end index. If end is omitted, selects all elements from start until the end of the array. Returns a list. |
| `[:n]`                   |                                                            Selects the first n elements of the array. Returns a list.                                                             |
| `[-n:]`                  |                                                             Selects the last n elements of the array. Returns a list.                                                             |
| `[-n]`                  |                                                             Selects the n-th elements counted from last of the array. Returns a list.                                                             |
| `[?(expression)]`        |                                Filter expression. Selects all elements in an object or array that filter expression is true for it. Returns a list.                                 |
| `[(expression)]`         |                                                 Script expression. Expression is evaluated as property names or indexes to select.                                                  |
| `@`                      |                                                      Used in filter expressions to refer to the current node being processed.                                                       |

List are represented in json array. To distinct between list and array node, use

`path_result json_path_get_distinct(json_t *json, const char *path);`

instead.

When a selection returns invalid list, it returns an empty list; When a selection returns invalid node is it returns NULL.

### Expression

| Operator | Description                                    |
| -------- | ---------------------------------------------- |
| `==`     | Equals to.                                     |
| `!=`     | Not equal to.                                  |
| `>`      | Greater than.                                  |
| `>=`     | Greater than or equal to.                      |
| `<`      | Less than.                                     |
| `<=`     | Less than or equal to.                         |
| `=~`     | Match a regular expression. Yet not supported. |
| `!`      | Negate an expression.                          |
| `&&`     | Logical AND.                                   |
| `\|\|`   | Logical OR.                                    |

Expressions are evaluated as json nodes.

Logical operators have the higher priority than comparisons and results can be used as oprand of another logical operator.

Comparisons only take paths and constants as oprand.

Brackets `(` `)` can be used.
