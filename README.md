# GT: Go To [somewhere]

GT is a CLI tool to simplify directory navigation in Unix-like systems. It allows you to traverse the directory tree without specifying intermediate directories, using only a partial name. What does this mean? Less typing!

## Features

- Fast directory search based on partial names
- Configurable search scope and exclusions
- Tab completion for interactive directory selection
- Support for both Bash and Zsh shells (theoretically — not fully tested in Bash)

## Installation

1. Clone the repository:
   ```
   git clone https://github.com/lnoir/gt.git
   cd gt
   ```

2. Compile and install the application:
   ```
   make
   make install
   ```

3. Add the following line to your `.zprofile` or `.bash_profile`:
   ```
   source /usr/local/share/gt/gts.sh
   ```

4. Reload your shell configuration:
   ```shell
   # For Zsh
   autoload -Uz compinit && compinit
   source /usr/local/share/gt/gts.sh
   ```
   Or:
   ```shell
   # For Bash
   source ~/.bash_profile
   ```

## Configuration

GT uses a configuration file located at `~/.config/gt/config`. If this file doesn't exist, GT will use default settings.

To configure GT, create the file and add the following options:

```
top_level_dir=/path/to/search/root
exclusions=node_modules,build,.git
```

- `top_level_dir`: Specifies the root directory for searches. You can add multiple top-level directories.
- `exclusions`: A comma-separated list of directory names to exclude from searches.

## Usage

### Basic Navigation

To navigate to a directory:

```
gt directory_name
```

GT will search for directories matching the given name and either navigate directly to it (if there's only one match) or present a list of options to choose from.

### Tab Completion

Start typing a directory name and press Tab to see matching options:

```
gt doc<TAB>
```

This will show a list of directories containing "doc" in their name.

### Searching Up the Directory Tree

To search in parent directories, use the `../` prefix:

```
gt ../project
```

This will search for directories named "project" in the current and parent directories.

## Commands

GT supports two main commands:

- `search`: Performs a broader search (default when using tab completion)
- `immediate`: Searches only in the immediate vicinity (used when directly calling `gt`)

These commands are used internally and don't need to be called directly by the user.

## Troubleshooting

If you encounter any issues:

1. Ensure that `gts` is in your PATH.
2. Check that the shell script is sourced correctly in your shell configuration file.
3. Verify that you have the necessary permissions to access the directories you're searching.

For more detailed debugging, you can enable debug mode by setting `GT_DEBUG=1` in the shell script.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

[Specify your license here]