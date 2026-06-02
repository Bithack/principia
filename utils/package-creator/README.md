# `package-creator`
Create a Principia package file from JSON data.

For more information about Principia level packages, see the [wiki](https://principia-web.se/wiki/Packages).

## Usage

```bash
package-creator <package-json> <package-file>
```

## Format
Anything except for `name` and `levels` is optional, the default values are the ones provided below:

```json
{
	"name": "Test Package",
	"community_id": 0,
	"levels_unlocked": 2,
	"first_is_menu": 0,
	"return_on_finish": 0,
	"levels": [
		67,
		68
	]
}
```
