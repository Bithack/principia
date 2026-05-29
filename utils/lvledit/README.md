# `lvledit`
Get and set various fields in the Principia level header.

## Usage

```bash
lvledit <levelpath> <method>
```

To check the maximum level version that lvledit is built to handle, use:

```bash
lvledit --get-built-level-version
```

### Methods
- `get-description`: Get level description
- `set-description`: Set level description
- `get-name`: Get level name
- `set-name`: Set level name
- `get-type`: Get level type
- `get-parent-id`: Get parent ID of level (when it has been derived)
- `get-version`: Get level version
- `get-revision`: Get revision of level
- `get-gids`: Get a list of g_id's that exist in the level
- `get-visibility`: Get the visibility state of the level
- `get-community-id`: Get the community ID of the level
- `set-community-id`: Set the community ID of the level
- `flag-active`: Get if level flag X is active

### Input and output
To input data to a setter method you will need to pipe it through stdout. For example:

```bash
echo -n "Level Title" | lvledit bla.plvl --set-name
```

Output for getters are printed to stdout. Nothing else will print to stdout, errors will be printed to stderr.
