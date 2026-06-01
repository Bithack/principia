# Featured List Creator
This is a C program that generates a featured list file (`fl.cache`) from JSON data and images, which is served by the Principia community site to display featured levels and getting started links in the main menu.

The program requires the jansson library for JSON parsing, and will attempt to statically link against it by default. You can do `make STATIC=0` to link dynamically instead.

## Usage
There are two optional arguments for specifying the input JSON file and output `fl.cache` file. The default values are below:

```bash
./featured-list-creator data/data.json fl.cache
```

## Format
The format of the input JSON file is as follows:

```json
{
	"featured_levels": [
		{
			"id": 1,
			"name": "Name",
			"author": "Author",
			"jpeg_image": "level_thumbnail.jpg"
		}
		[...]
	],

	"gettingstarted_list": [
		{
			"name": "Link 1",
			"link": "https://example.org"
		}
		[...]
	]
}
```

The paths to the JPEG images are relative to the current working directory when running the program.
