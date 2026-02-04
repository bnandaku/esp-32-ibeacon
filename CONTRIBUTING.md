# Contributing to ESP32 iBeacon Transmitter

Thank you for your interest in contributing! This document provides guidelines for contributing to this project.

## Code of Conduct

- Be respectful and inclusive
- Welcome newcomers and help them learn
- Focus on constructive feedback
- Assume good intentions

## How to Contribute

### Reporting Bugs

Before submitting a bug report:
- Check existing issues to avoid duplicates
- Verify the issue with the latest version
- Collect relevant information (ESP-IDF version, board model, etc.)

When submitting:
- Use a clear, descriptive title
- Describe the expected vs actual behavior
- Provide steps to reproduce
- Include serial output/logs if applicable
- Mention your environment (OS, ESP-IDF version, board)

### Suggesting Features

- Use GitHub Discussions for feature ideas
- Explain the use case and benefits
- Consider backward compatibility
- Be open to feedback and alternatives

### Pull Requests

#### Before You Start
- Open an issue first to discuss major changes
- Check that your change aligns with project goals
- Ensure it doesn't duplicate existing work

#### Development Process

1. **Fork and clone**
   ```bash
   git clone https://github.com/yourusername/esp32-ibeacon.git
   cd esp32-ibeacon
   ```

2. **Create a branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow the existing code style
   - Add comments for complex logic
   - Keep functions small and focused
   - Test on real hardware when possible

4. **Test your changes**
   ```bash
   idf.py build
   idf.py flash monitor
   ```

5. **Commit your changes**
   ```bash
   git add .
   git commit -m "Add feature: brief description"
   ```

   Commit message guidelines:
   - Use present tense ("Add feature" not "Added feature")
   - Be concise but descriptive
   - Reference issue numbers when applicable

6. **Push and create PR**
   ```bash
   git push origin feature/your-feature-name
   ```

   Then open a Pull Request on GitHub

#### Pull Request Guidelines

- Fill out the PR template completely
- Link related issues
- Describe what changed and why
- Include testing steps
- Update documentation if needed
- Keep PRs focused on a single change

## Code Style

### C Code
- Use 4 spaces for indentation (no tabs)
- Keep lines under 100 characters when possible
- Use descriptive variable names
- Add comments for non-obvious code
- Follow ESP-IDF coding conventions

### Documentation
- Update README.md for user-facing changes
- Add inline code comments for developers
- Use clear, simple language
- Include examples where helpful

## Testing

- Test on actual ESP32 hardware when possible
- Verify with multiple ESP32 variants (ESP32, ESP32-C3, ESP32-S3)
- Check that beacons are detected by iOS/Android apps
- Monitor serial output for errors
- Test different configuration options

## Project Structure

```
esp32-ibeacon/
├── main/
│   ├── main.c              # Main application code
│   ├── esp_ibeacon_api.c   # iBeacon protocol implementation
│   ├── esp_ibeacon_api.h   # API header
│   └── CMakeLists.txt
├── CMakeLists.txt
├── README.md
├── LICENSE
└── CONTRIBUTING.md
```

## Questions?

- Check the [README](README.md) first
- Search existing issues
- Open a discussion for general questions
- Tag maintainers if urgent

## Recognition

Contributors will be:
- Listed in the project's acknowledgments
- Mentioned in release notes
- Credited in commit history

Thank you for making this project better! 🚀
