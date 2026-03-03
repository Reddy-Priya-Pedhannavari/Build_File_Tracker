# Contributing to BuildFileTracker

Thank you for your interest in contributing to BuildFileTracker! This document provides guidelines and instructions for contributing.

## Code of Conduct

Be respectful, inclusive, and professional in all interactions.

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in [Issues](https://github.com/yourusername/buildfiletracker/issues)
2. If not, create a new issue with:
   - Clear title and description
   - Steps to reproduce
   - Expected vs actual behavior
   - OS and build system information
   - Relevant logs or error messages

### Suggesting Features

1. Check existing [Issues](https://github.com/yourusername/buildfiletracker/issues) for similar requests
2. Create a new issue with:
   - Clear use case description
   - Proposed solution or implementation
   - Benefits and potential drawbacks

### Code Contributions

#### Getting Started

1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/yourusername/buildfiletracker.git
   cd buildfiletracker
   ```
3. Create a feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```

#### Development Setup

```bash
# Build the library
cd src
make

# Install Python dependencies
cd ../python
pip install -r requirements.txt

# Run examples to test
cd ../examples/cmake_example
mkdir build && cd build
cmake ..
make
```

#### Coding Standards

**C Code:**
- Follow K&R style
- Use meaningful variable names
- Comment complex logic
- Keep functions focused and small
- Check return values
- Handle errors gracefully

**Python Code:**
- Follow PEP 8
- Use type hints where appropriate
- Write docstrings for functions
- Keep functions under 50 lines when possible

**General:**
- Write clear commit messages
- Add tests for new features
- Update documentation
- Keep PRs focused on one change

#### Making Changes

1. Write your code
2. Test thoroughly:
   ```bash
   # Test C library
   cd src && make clean && make
   
   # Test with examples
   cd ../examples/cmake_example
   ./integrations/track_build.sh make
   
   # Test Python tools
   python3 python/report_generator.py report.json -f all
   ```
3. Update documentation if needed
4. Commit with clear messages:
   ```bash
   git commit -m "Add feature: description of feature"
   ```

#### Submitting Pull Requests

1. Push your branch:
   ```bash
   git push origin feature/your-feature-name
   ```
2. Create a Pull Request with:
   - Clear title and description
   - Reference to related issues
   - List of changes
   - Testing performed
3. Respond to review feedback
4. Make requested changes
5. Wait for approval and merge

### Documentation

Documentation improvements are always welcome!

- Fix typos or unclear explanations
- Add examples
- Improve installation instructions
- Update integration guides
- Add FAQ entries

## Development Guidelines

### Adding New Features

1. **File Interception**: Modify `src/file_interceptor.c`
2. **Tracking Logic**: Modify `src/file_tracker.c`
3. **Report Formats**: Modify `python/report_generator.py`
4. **Analysis Tools**: Modify `python/analyzer.py`
5. **Integrations**: Add to `integrations/`

### Testing

Before submitting:
1. Build cleanly with no warnings
2. Test with at least one example
3. Verify reports generate correctly
4. Check for memory leaks (if possible):
   ```bash
   valgrind --leak-check=full ./your_program
   ```

### Commit Message Format

```
<type>: <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Test additions/changes
- `chore`: Build/maintenance tasks

**Example:**
```
feat: Add XML report generation

- Implement XML output in report_generator.py
- Add XML parsing tests
- Update documentation

Closes #123
```

## Community

- Join discussions in [GitHub Discussions](https://github.com/yourusername/buildfiletracker/discussions)
- Ask questions in Issues (label: question)
- Share your use cases and experiences

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Recognition

Contributors will be recognized in:
- README.md acknowledgments
- Release notes
- GitHub contributors page

Thank you for contributing to BuildFileTracker!
