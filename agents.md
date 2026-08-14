# Wallpaper Manager for Windows - Agent Definitions

This document defines the AI agents, their responsibilities, capabilities, and workflows for implementing the Wallpaper Manager for Windows application. Each agent is specialized for specific tasks within the project architecture.

---

## Agent Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Orchestrator Agent                        │
│              (Project Coordination & Planning)               │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  Code Agent   │   │   Testing     │   │   Document    │
│  (Primary)    │   │   Agent       │   │   Agent       │
└───────────────┘   └───────────────┘   └───────────────┘
        │
        ├──────────┬──────────┬──────────┬──────────┐
        │          │          │          │          │
        ▼          ▼          ▼          ▼          ▼
   ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
   │ grim-  │ │ slurp- │ │ awww-  │ │  GUI   │ │  DB    │
   │ win    │ │ win    │ │ win    │ │ Agent  │ │ Agent  │
   │ Agent  │ │ Agent  │ │ Agent  │ │        │ │        │
   └────────┘ └────────┘ └────────┘ └────────┘ └────────┘
```

---

## 1. Orchestrator Agent

### Role
Project coordinator and task manager responsible for overall planning, progress tracking, and ensuring all components integrate correctly.

### Responsibilities
- Break down high-level requirements into actionable tasks
- Assign tasks to specialized agents
- Monitor progress and adjust timelines
- Ensure cross-component compatibility
- Manage dependencies between modules
- Coordinate testing and integration phases

### Capabilities
- Project planning and timeline management
- Risk assessment and mitigation planning
- Resource allocation
- Progress reporting
- Conflict resolution between components

### Input/Output
**Input:**
- User requirements
- Project constraints
- Progress reports from other agents

**Output:**
- Task assignments
- Updated project plan
- Integration schedules
- Status reports

### Example Workflow
```yaml
Task: Implement wallpaper manager
Steps:
  1. Analyze requirements from plan.md
  2. Create component implementation order
  3. Assign grim-win implementation to Code Agent
  4. Wait for completion and review
  5. Assign slurp-win implementation
  6. Coordinate integration testing
  7. Report progress to user
```

### Prompt Template
```
You are the Orchestrator Agent for the Wallpaper Manager Windows project.

Current Phase: {phase_name}
Completed Components: {completed_list}
Pending Components: {pending_list}
Blockers: {blocker_list}

Your task is to:
1. Review the current state
2. Identify the next critical path item
3. Assign it to the appropriate specialist agent
4. Define acceptance criteria
5. Set realistic deadlines

Reference Documents:
- /workspace/plan.md (project plan)
- /workspace/agents.md (this file)
```

---

## 2. Code Agent (Primary)

### Role
Main code generation agent responsible for implementing core functionality across all modules.

### Responsibilities
- Write production-quality Python code
- Follow project structure and coding standards
- Implement error handling and logging
- Create type hints and documentation strings
- Optimize for performance
- Ensure Windows compatibility

### Capabilities
- Python programming (async/await, OOP, functional)
- Windows API integration (ctypes, winreg)
- GUI development (PyQt6/tkinter)
- Database design and queries
- REST API integration
- Image processing

### Specializations
The Code Agent can delegate to specialized sub-agents:

#### 2.1 grim-win Agent (Screenshot Engine)

**Focus**: Screenshot capture functionality

**Expertise Required:**
- `mss` library for screen capture
- PIL/Pillow for image processing
- Multi-monitor handling
- Win32 API for window enumeration
- Performance optimization for fast captures

**Tasks:**
1. Implement `ScreenshotEngine` class
2. Add full monitor capture
3. Add region-based capture
4. Add window-specific capture
5. Implement multi-monitor support
6. Add image format conversion
7. Create async capture methods
8. Write unit tests for capture functions

**Example Implementation Task:**
```python
# Task: Implement capture_region method
Requirements:
- Accept coordinates (left, top, width, height)
- Support multiple output formats (PNG, JPEG, WebP)
- Return bytes and optionally save to file
- Handle errors gracefully
- Must work across all monitors

Acceptance Criteria:
✓ Captures exact region specified
✓ No memory leaks after 1000 captures
✓ < 100ms for 1920x1080 region
✓ Supports negative coordinates for multi-monitor
```

**Prompt Template:**
```
You are the grim-win Specialist Agent.

Task: Implement screenshot capture functionality for Windows
Reference: plan.md Section 2.1 (grim-win: Screenshot Engine)

Technical Requirements:
- Use mss library for capture
- PIL for image processing
- Support multi-monitor setups
- Output formats: PNG, JPEG, WebP

Implementation Location: src/core/screenshot.py

Constraints:
- Must be async-compatible
- Memory efficient (no leaks)
- Thread-safe operations
- Proper error handling

Test Cases to Consider:
1. Single monitor capture
2. Multi-monitor capture
3. Region spanning multiple monitors
4. Invalid coordinates
5. Permission issues
```

#### 2.2 slurp-win Agent (Region Selector)

**Focus**: Interactive region selection UI

**Expertise Required:**
- tkinter or PyQt6 for overlay windows
- Event handling (mouse, keyboard)
- Multi-monitor coordinate systems
- Visual feedback rendering
- User experience design

**Tasks:**
1. Implement `RegionSelectorOverlay` class
2. Create full-screen transparent overlay
3. Implement click-and-drag selection
4. Add visual rectangle highlighting
5. Display real-time coordinates
6. Handle keyboard shortcuts (Enter, Esc)
7. Detect monitor boundaries
8. Return selected region data

**Example Implementation Task:**
```python
# Task: Implement region selection overlay
Requirements:
- Full-screen semi-transparent overlay
- Cross-hair cursor
- Click-drag to select rectangle
- Green border on selection
- Real-time coordinate display
- ESC to cancel, ENTER to confirm

Acceptance Criteria:
✓ Works on all monitor configurations
✓ Selection visible and clear
✓ Coordinates accurate
✓ Smooth drag operation
✓ Clean exit on cancel/confirm
```

**Prompt Template:**
```
You are the slurp-win Specialist Agent.

Task: Implement interactive region selection tool for Windows
Reference: plan.md Section 2.2 (slurp-win: Region Selector)

Technical Requirements:
- Full-screen overlay window
- Mouse event handling (press, drag, release)
- Keyboard shortcuts (Enter=confirm, Esc=cancel)
- Real-time coordinate display
- Multi-monitor awareness

Implementation Location: src/core/region_selector.py

UI/UX Requirements:
- Semi-transparent gray overlay (alpha=0.3)
- Cross-hair cursor
- Bright green selection rectangle (#00ff00)
- Black info box with white text
- Smooth rendering (no flicker)

Edge Cases:
- Selection across monitor boundaries
- Very small selections (< 10x10)
- Very large selections (full screen)
- Rapid mouse movements
```

#### 2.3 awww-win Agent (Wallpaper Downloader)

**Focus**: Wallpaper discovery and download

**Expertise Required:**
- Async HTTP requests (aiohttp)
- REST API integration
- Web scraping (if needed)
- File I/O and caching
- Rate limiting and retry logic
- Authentication (API keys)

**Tasks:**
1. Implement `WallpaperDownloader` class
2. Integrate Wallhaven.cc API
3. Integrate Unsplash API
4. Add anime wallpaper sources
5. Implement search with filters
6. Add batch download capability
7. Create caching system
8. Track download progress
9. Handle rate limits

**Example Implementation Task:**
```python
# Task: Implement wallpaper search and download
Requirements:
- Search multiple sources (Wallhaven, Unsplash)
- Filter by resolution, tags, categories
- Download with progress tracking
- Cache downloaded files
- Handle API rate limits
- Support batch operations

Acceptance Criteria:
✓ Returns relevant results
✓ Respects API rate limits
✓ Downloads complete without corruption
✓ Cache prevents duplicate downloads
✓ Progress callbacks work correctly
```

**Prompt Template:**
```
You are the awww-win Specialist Agent.

Task: Implement wallpaper discovery and download engine
Reference: plan.md Section 2.3 (awww-win: Wallpaper Downloader)

API Sources to Integrate:
1. Wallhaven.cc (primary - anime focus)
2. Unsplash (general photography)
3. Anime-Wallpapers.com (anime specific)

Technical Requirements:
- Async HTTP with aiohttp
- API key management
- Response parsing and validation
- File download with progress
- Smart caching (hash-based)
- Concurrent downloads (limit=5)

Rate Limiting:
- Wallhaven: 1 request/second
- Unsplash: 50 requests/hour
- Implement exponential backoff

Implementation Location: src/core/downloader.py
```

#### 2.4 GUI Agent (User Interface)

**Focus**: Application graphical interface

**Expertise Required:**
- PyQt6 or CustomTkinter
- Modern UI design principles
- Event-driven programming
- Theming and styling
- Responsive layouts
- System tray integration

**Tasks:**
1. Design main application window
2. Implement wallpaper gallery grid
3. Create preview panel
4. Build settings dialog
5. Add search/filter UI
6. Implement drag-and-drop
7. Create context menus
8. Add system tray icon
9. Implement notification system

**Example Implementation Task:**
```python
# Task: Implement main window with gallery view
Requirements:
- Dark/light theme toggle
- Grid layout for wallpapers (responsive)
- Thumbnail loading (lazy)
- Selection highlighting
- Right-click context menu
- Search bar at top
- Status bar at bottom

Acceptance Criteria:
✓ Smooth scrolling with 1000+ items
✓ Thumbnails load progressively
✓ Theme changes apply instantly
✓ Keyboard navigation works
✓ Window remembers size/position
```

**Prompt Template:**
```
You are the GUI Specialist Agent.

Task: Implement main application user interface
Reference: plan.md Section 4 (GUI Development)

Technology Stack:
- Primary: PyQt6 with QML (preferred for modern look)
- Alternative: CustomTkinter (simpler, good enough)

Key Screens to Implement:
1. Main window with gallery view
2. Wallpaper preview panel
3. Settings dialog
4. About/Help window
5. First-run wizard

Design Requirements:
- Modern, clean aesthetic
- Dark mode by default
- Responsive to window resize
- High DPI support
- Accessibility considerations

Implementation Location: src/gui/
```

#### 2.5 DB Agent (Database & State Management)

**Focus**: Data persistence and retrieval

**Expertise Required:**
- SQLite database design
- ORM patterns (or raw SQL)
- Migration management
- Query optimization
- Data modeling
- Backup/restore

**Tasks:**
1. Design database schema
2. Implement repository pattern
3. Create migration scripts
4. Add CRUD operations
5. Implement search indexing
6. Add backup functionality
7. Optimize queries
8. Handle concurrent access

**Example Implementation Task:**
```python
# Task: Implement wallpaper repository
Requirements:
- CRUD operations for wallpapers
- Tag management
- Playlist support
- Efficient search (full-text)
- Statistics tracking (use count, rating)
- Soft delete capability

Acceptance Criteria:
✓ All queries < 50ms for 10K records
✓ ACID compliance
✓ Handles concurrent writes
✓ Automatic migrations
✓ Transaction support
```

**Prompt Template:**
```
You are the Database Specialist Agent.

Task: Implement data persistence layer
Reference: plan.md "Database Schema" section

Database: SQLite 3.x

Key Tables:
- wallpapers (main content)
- tags (categorization)
- wallpaper_tags (junction)
- playlists (collections)
- playlist_items (playlist contents)
- monitor_config (per-monitor settings)
- settings (application config)

Implementation Pattern: Repository
- Abstract data access
- Easy to test
- Swap implementations if needed

Implementation Location: src/database/
```

---

## 3. Testing Agent

### Role
Quality assurance and test creation specialist.

### Responsibilities
- Write comprehensive unit tests
- Create integration test suites
- Perform end-to-end testing
- Measure code coverage
- Identify edge cases
- Performance benchmarking
- Regression testing

### Capabilities
- pytest framework
- Mocking and fixtures
- Test automation
- CI/CD integration
- Performance profiling
- Memory leak detection

### Test Categories

#### Unit Tests
```python
# Example: Testing screenshot engine
def test_capture_region_valid_coordinates():
    engine = ScreenshotEngine()
    result = engine.capture_region(0, 0, 100, 100)
    assert isinstance(result, bytes)
    assert len(result) > 0

def test_capture_region_invalid_coordinates():
    engine = ScreenshotEngine()
    with pytest.raises(ValueError):
        engine.capture_region(-10000, -10000, 0, 0)

def test_capture_memory_leak():
    engine = ScreenshotEngine()
    initial_memory = get_process_memory()
    for _ in range(1000):
        engine.capture_region(0, 0, 100, 100)
    final_memory = get_process_memory()
    assert final_memory - initial_memory < 10 * 1024 * 1024  # < 10MB leak
```

#### Integration Tests
```python
# Example: Full workflow test
@pytest.mark.integration
async def test_full_workflow():
    # 1. Select region (mocked)
    region = SelectedRegion(0, 0, 1920, 1080, 0)
    
    # 2. Capture screenshot
    engine = ScreenshotEngine()
    image_bytes = engine.capture_region(**region.as_dict())
    
    # 3. Save temporarily
    temp_path = save_temp(image_bytes)
    
    # 4. Set as wallpaper
    wallpaper_engine = WallpaperEngine()
    success = wallpaper_engine.set_wallpaper(temp_path)
    
    assert success
    cleanup(temp_path)
```

#### End-to-End Tests
```python
# Example: E2E user scenario
@pytest.mark.e2e
def test_user_downloads_and_sets_wallpaper():
    app = launch_application()
    
    # Search for wallpaper
    app.search("anime landscape")
    
    # Select first result
    app.select_wallpaper(0)
    
    # Download
    app.download()
    
    # Set as wallpaper
    app.set_as_wallpaper()
    
    # Verify
    current = get_current_wallpaper()
    assert current.exists()
    
    app.close()
```

### Prompt Template
```
You are the Testing Agent.

Component to Test: {component_name}
Location: {file_path}
Critical Functions: {function_list}

Create comprehensive tests covering:
1. Happy path scenarios
2. Edge cases and boundary conditions
3. Error handling
4. Performance benchmarks
5. Memory usage
6. Thread safety (if applicable)

Test Framework: pytest
Coverage Target: >80%
Mock External: APIs, file system, Windows registry

Output: Complete test file ready to run
```

---

## 4. Documentation Agent

### Role
Technical writer and documentation specialist.

### Responsibilities
- Write API documentation
- Create user guides
- Generate README files
- Document installation process
- Write troubleshooting guides
- Create video tutorial scripts
- Maintain changelog

### Documentation Types

#### API Documentation
```markdown
## ScreenshotEngine

### capture_region(left, top, width, height, output_path=None, format='png')

Capture a specific region of the screen.

**Parameters:**
- `left` (int): X coordinate of top-left corner
- `top` (int): Y coordinate of top-left corner
- `width` (int): Width of region in pixels
- `height` (int): Height of region in pixels
- `output_path` (str, optional): File path to save image
- `format` (str): Image format ('png', 'jpg', 'webp')

**Returns:**
bytes: Image data

**Raises:**
- `ValueError`: If coordinates are invalid
- `PermissionError`: If screen capture is blocked

**Example:**
```python
engine = ScreenshotEngine()
image = engine.capture_region(0, 0, 1920, 1080)
```
```

#### User Guide
```markdown
# Getting Started with Wallpaper Manager

## Installation

1. Download the installer from [releases]
2. Run `WallpaperManager-Setup.exe`
3. Follow the installation wizard
4. Launch from Start Menu

## Quick Start

### Setting Your First Wallpaper

1. Open Wallpaper Manager
2. Click "Browse" or press `Ctrl+O`
3. Select an image from your computer
4. Click "Set as Wallpaper"

### Using Region Selection

1. Press `Win+Shift+R` or click the crop icon
2. Click and drag to select a region
3. Press Enter to confirm or Esc to cancel
4. The selected region becomes your wallpaper
```

#### Troubleshooting Guide
```markdown
# Troubleshooting

## Common Issues

### "Cannot capture screen" error

**Cause:** Screen capture permissions not granted

**Solution:**
1. Open Windows Settings
2. Go to Privacy → Screen capture
3. Enable for Wallpaper Manager

### Wallpapers not downloading

**Cause:** API rate limit or network issue

**Solution:**
1. Check internet connection
2. Wait 5 minutes and retry
3. Configure your own API keys in Settings
```

### Prompt Template
```
You are the Documentation Agent.

Document Type: {api_guide|user_manual|readme|troubleshooting}
Target Audience: {developers|end_users|sysadmins}
Subject: {component_or_feature}

Requirements:
- Clear, concise language
- Include examples
- Cover common pitfalls
- Link to related docs
- SEO-friendly (for public docs)

Style Guide:
- Active voice
- Present tense
- Second person ("you")
- Consistent terminology
```

---

## 5. Security Agent

### Role
Security auditing and vulnerability prevention.

### Responsibilities
- Review code for security issues
- Validate input handling
- Check for injection vulnerabilities
- Audit API key storage
- Review network communications
- Ensure secure defaults

### Security Checklist

```yaml
Code Review Points:
  Input Validation:
    ✓ All user inputs sanitized
    ✓ File paths validated (no traversal)
    ✓ URLs validated before requests
    ✓ SQL queries parameterized
    
  Authentication:
    ✓ API keys stored securely
    ✓ No hardcoded credentials
    ✓ Keys encrypted at rest
    
  Network:
    ✓ HTTPS for all external calls
    ✓ Certificate validation enabled
    ✓ No sensitive data in logs
    
  File Operations:
    ✓ Safe file writing (atomic)
    ✓ Proper permissions
    ✓ Temp files cleaned up
    
  Memory:
    ✓ No buffer overflows
    ✓ Proper resource cleanup
    ✓ No sensitive data in memory longer than needed
```

### Prompt Template
```
You are the Security Agent.

Reviewing: {file_or_component}
Threat Model: {description}

Perform security audit focusing on:
1. Injection vulnerabilities (SQL, command, path)
2. Authentication/authorization issues
3. Data exposure risks
4. Insecure dependencies
5. Configuration weaknesses

Provide:
- Vulnerability description
- Severity rating (CVSS)
- Exploit scenario
- Remediation steps
- Code fix example
```

---

## Agent Collaboration Workflows

### Workflow 1: New Feature Implementation

```
┌─────────────┐
│Orchestrator │
│   Agent     │
└──────┬──────┘
       │ 1. Receive feature request
       ▼
┌─────────────┐
│  Analyze &  │
│   Plan      │
└──────┬──────┘
       │ 2. Break into tasks
       ▼
┌─────────────────────────────────────┐
│         Assign to Specialists        │
│  ┌──────────┐  ┌──────────┐         │
│  │ Code     │  │ Testing  │         │
│  │ Agent    │  │ Agent    │         │
│  └────┬─────┘  └────┬─────┘         │
│       │             │                │
│       │ 3. Implement│                │
│       ▼             │                │
│  ┌──────────┐       │                │
│  │ Review   │◄──────┤                │
│  │ Code     │       │                │
│  └────┬─────┘       │                │
│       │             │                │
│       │ 4. Write    │                │
│       └────────────►│                │
│                     │ 5. Test        │
│                     │                │
│                     ▼                │
│                ┌──────────┐          │
│                │ Fix      │          │
│                │ Issues   │          │
│                └────┬─────┘          │
│                     │                │
│                     │ 6. Approve     │
│                     ▼                │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────┐
│ Document    │
│ Agent       │
└──────┬──────┘
       │ 7. Update docs
       ▼
┌─────────────┐
│   Feature   │
│  Complete   │
└─────────────┘
```

### Workflow 2: Bug Fix Process

```
User Reports Bug
       │
       ▼
┌─────────────┐
│Orchestrator │
│   Agent     │
└──────┬──────┘
       │ Triage
       ▼
┌─────────────┐
│ Reproduce & │
│   Isolate   │
└──────┬──────┘
       │
       ├─────────────┬─────────────┐
       ▼             ▼             ▼
┌──────────┐ ┌──────────┐ ┌──────────┐
│ Code     │ │ Testing  │ │ Security │
│ Agent    │ │ Agent    │ │ Agent    │
│ (Fix)    │ │ (Verify) │ │ (Audit)  │
└────┬─────┘ └────┬─────┘ └────┬─────┘
     │            │            │
     └────────────┴────────────┘
                  │
                  ▼
         ┌─────────────┐
         │  All Pass?  │
         └──────┬──────┘
                │
        Yes ────┴──── No
        │                │
        ▼                ▼
┌─────────────┐   ┌─────────────┐
│  Document   │   │  Iterate    │
│   Fix in    │   │   on Fix    │
│ Changelog   │   │             │
└─────────────┘   └─────────────┘
```

---

## Agent Communication Protocol

### Message Format

```json
{
  "message_id": "uuid",
  "timestamp": "ISO8601",
  "sender": "agent_name",
  "recipient": "agent_name",
  "type": "task_assignment|status_update|question|response|alert",
  "priority": "low|normal|high|critical",
  "content": {
    "subject": "Brief subject",
    "body": "Detailed message",
    "attachments": ["file_paths"],
    "action_required": true,
    "deadline": "ISO8601"
  },
  "thread_id": "uuid",
  "in_reply_to": "message_id"
}
```

### Status Update Format

```json
{
  "agent": "grim-win-agent",
  "task": "Implement capture_region",
  "status": "in_progress",
  "progress_percent": 65,
  "current_activity": "Writing unit tests",
  "blockers": [],
  "estimated_completion": "2024-01-15T14:00:00Z",
  "notes": "On track, no issues"
}
```

---

## Quality Gates

### Code Review Checklist

Before any code is merged, the following must be verified:

```yaml
Code Quality:
  - PEP 8 compliance: ✓
  - Type hints present: ✓
  - Docstrings complete: ✓
  - Error handling: ✓
  - Logging appropriate: ✓

Testing:
  - Unit tests written: ✓
  - Integration tests: ✓
  - Coverage >80%: ✓
  - All tests pass: ✓

Performance:
  - No obvious inefficiencies: ✓
  - Memory usage acceptable: ✓
  - Async where beneficial: ✓

Security:
  - Input validation: ✓
  - No hardcoded secrets: ✓
  - Dependencies audited: ✓

Documentation:
  - API docs updated: ✓
  - User guide reflects changes: ✓
  - Changelog entry: ✓
```

---

## Agent-Specific Prompt Engineering

### Best Practices for Agent Prompts

1. **Be Specific**: Clearly define the task scope
2. **Provide Context**: Reference plan.md sections
3. **Set Constraints**: Define boundaries and limitations
4. **Include Examples**: Show expected output format
5. **Define Success**: State acceptance criteria clearly

### Example Comprehensive Prompt

```
ROLE: You are the grim-win Specialist Agent

TASK: Implement the ScreenshotEngine.capture_window() method

CONTEXT:
- Part of Wallpaper Manager for Windows project
- Equivalent to grim's window capture on Linux
- Will be used when users want to capture a specific application window

REQUIREMENTS:
1. Find window by title using Win32 API
2. Get window coordinates and dimensions
3. Capture only the window content (not borders)
4. Handle minimized/invisible windows gracefully
5. Support both synchronous and async calls

TECHNICAL DETAILS:
- Use win32gui.FindWindow() for window lookup
- Use win32gui.GetWindowRect() for coordinates
- Use mss for actual capture
- Return bytes in specified format

EDGE CASES TO HANDLE:
- Window not found
- Window minimized
- Window behind another window
- Multiple windows with same title
- Special characters in window title

ACCEPTANCE CRITERIA:
✓ Method signature matches spec
✓ Returns valid image bytes
✓ Raises appropriate exceptions
✓ Works on Windows 10/11
✓ < 200ms for typical window
✓ No memory leaks

REFERENCE:
- plan.md Section 2.1
- Existing capture_full() and capture_region() implementations
- Windows API documentation

OUTPUT:
Complete implementation in src/core/screenshot.py with docstring and type hints
```

---

## Continuous Improvement

### Retrospective Process

After each major milestone:

1. **Gather Data**: Collect metrics on:
   - Time spent per component
   - Bug count and severity
   - Code review iterations
   - Test coverage trends

2. **Identify Patterns**: Look for:
   - Recurring issues
   - Bottlenecks
   - Successful strategies
   - Knowledge gaps

3. **Adjust Processes**: Update:
   - Agent prompts based on what worked
   - Quality gates if gaps found
   - Documentation templates
   - Testing strategies

4. **Update This Document**: Revise agents.md with:
   - New agent roles if needed
   - Improved workflows
   - Better prompt templates
   - Lessons learned

---

## Appendix: Quick Reference

### Agent Directory

| Agent | Specialty | Location | Key Files |
|-------|-----------|----------|-----------|
| Orchestrator | Planning | N/A | plan.md |
| Code (Primary) | Implementation | src/ | All .py files |
| grim-win | Screenshots | src/core/ | screenshot.py |
| slurp-win | Region Select | src/core/ | region_selector.py |
| awww-win | Downloads | src/core/ | downloader.py |
| GUI | UI/UX | src/gui/ | *.py in gui/ |
| DB | Database | src/database/ | models.py, repository.py |
| Testing | QA | tests/ | test_*.py |
| Documentation | Docs | docs/ | *.md |
| Security | Auditing | N/A | Reviews all code |

### Command Quick Reference

```bash
# Setup development environment
python -m venv .venv
source .venv/bin/activate  # or .venv\Scripts\Activate on Windows
pip install -r requirements.txt

# Run tests
pytest tests/ -v --cov=src

# Run specific test
pytest tests/test_screenshot.py::test_capture_region

# Check code quality
flake8 src/
mypy src/
black --check src/

# Build executable
pyinstaller --name WallpaperManager src/main.py
```

### File Naming Conventions

```
src/
  core/
    screenshot.py       # grim-win implementation
    region_selector.py  # slurp-win implementation
    downloader.py       # awww-win implementation
    wallpaper_engine.py # Windows wallpaper setting
  
  gui/
    main_window.py
    gallery_view.py
    settings_dialog.py
  
  database/
    models.py
    repository.py
    migrations/

tests/
  unit/
    test_screenshot.py
    test_region_selector.py
    test_downloader.py
  
  integration/
    test_workflow.py
  
  e2e/
    test_user_scenarios.py
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2024-01-XX | Initial agent definitions |

---

*This document should be reviewed and updated regularly as the project evolves and lessons are learned.*
