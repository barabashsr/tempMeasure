# CLAUDE.md - Embedded Development Configuration

> **Version:** 3.0 | **Target:** Solo embedded development with PlatformIO | **Last Updated:** 2025-01-20

## 🏃 TL;DR Quick Start

```bash
# First time only: Run setup script from "Claude Code Initial Setup Script" artifact
./setup_claude.sh

# Every session:
.claude/scripts/start_session.sh    # Creates baseline commit & docs
# ... do your work ...
.claude/scripts/archive_session.sh  # Archives session when done
```

## 🎯 Core Directives

1. **ALWAYS create baseline commit BEFORE any code changes**
2. **NEVER flush or rewrite planning documents - append only**
3. **ALWAYS add Doxygen comments to all new/modified code**
4. **READ DOCUMENTATION FIRST** - Use generated Doxygen docs instead of source files

## 🚀 Quick Start Checklist

```bash
# MANDATORY at EVERY session start:
# 1. Baseline commit
git add -A && git commit -m "baseline: pre-session $(date +%Y%m%d_%H%M%S)"

# 2. Start session with automated script
.claude/scripts/start_session.sh
# OR manually:
scripts/generate_docs.sh  # Generates docs with statistics

# 3. Verify environment
git branch --show-current  # Should be 'claude-branch'
/permissions              # Check MCP tools

# 4. Session context is auto-loaded by start_session.sh
# Or manually check:
cat .claude/sessions/current/DOC_SUMMARY.md
```

## 📋 Session Workflow

### 1. Session Initialization
```bash
# RECOMMENDED: Use automated session start
.claude/scripts/start_session.sh

# This script automatically:
# - Creates session directory with timestamp
# - Generates Doxygen documentation
# - Creates DOC_SUMMARY.md with class count
# - Sets up current session symlink

# OR Manual approach:
SESSION_ID=$(date +%Y%m%d_%H%M%S)
mkdir -p .claude/sessions/current

# Link current session
ln -sfn .claude/sessions/$SESSION_ID .claude/sessions/current

# Initialize session log
cat > .claude/sessions/current/STATUS.md << EOF
# Session: $SESSION_ID
## Started: $(date)
## Task: [Pending user input]
## Status: Initialized
EOF

# Generate documentation with statistics
scripts/generate_docs.sh  # Shows HTML count, classes, warnings
```

### 2. Documentation-First Onboarding (3 min max)
**READ generated docs INSTEAD of source code:**
1. Check `docs/doxygen/html/files.html` for project structure
2. Read `docs/doxygen/html/annotated.html` for class overview
3. Search relevant modules in `docs/doxygen/html/search/`
4. ONLY read source files if docs are insufficient

**Investigation approach for finding relevant files:**
```bash
# Use automated doc extraction
python3 scripts/extract_docs.py  # Lists all classes and methods

# Or use documentation search
grep -r "keyword" docs/doxygen/html/ --include="*.html" | head -10

# Find specific class methods
grep -A10 "Public Member Functions" docs/doxygen/html/class*.html

# If needed, use code search (but prefer docs)
grep -r "keyword" src/ --include="*.h" | head -5
```

### 3. Planning Phase (Documentation-Driven)
**APPEND to session plan in `.claude/sessions/current/PLAN.md`:**
```markdown
## Planning Update: [TIMESTAMP]
### Investigation Results:
- Relevant classes: [From doxygen docs]
- Key functions: [From doxygen docs]
- Dependencies: [From doxygen docs]

### Implementation Steps:
- [ ] Step 1: [Specific class::method to modify]
- [ ] Step 2: [Test to add/run]
- [ ] Step 3: [Documentation to regenerate]
```

### 4. Implementation Phase
**For EVERY code change:**
1. Check existing documentation first
2. Add/Update Doxygen comments
3. Build: `pio run -e <env>`
4. Regenerate docs: `doxygen Doxyfile`
5. Commit with documentation

**Doxygen Standards (MANDATORY):**
```cpp
/**
 * @file FileName.cpp
 * @brief One line description
 * @author Claude Code Session [SESSION_ID]
 * @date [DATE]
 * @details Extended description if needed
 * 
 * @section dependencies Dependencies
 * - List key dependencies
 * 
 * @section hardware Hardware Requirements
 * - List hardware if applicable
 */

/**
 * @brief Function purpose
 * @param[in] param1 Input parameter description
 * @param[out] param2 Output parameter description
 * @return What it returns
 * @retval 0 Success
 * @retval -1 Error condition
 * @note Implementation notes
 * @warning Any warnings
 * @see RelatedFunction()
 */
```

### 5. Documentation & Commit Protocol
```bash
# After EACH successful change:
# 1. Regenerate documentation with statistics
scripts/generate_docs.sh  # Shows file count and warnings

# 2. Commit with docs (git hook auto-generates if configured)
git add -A
git commit -m "feat(<module>): <what changed> 

- Added: [what was added]
- Modified: [what was modified]  
- Docs: Updated Doxygen for [which files]
- Tested: [how it was tested]"

# Note: If git pre-commit hook is installed, docs regenerate automatically

# 3. Update session status
cat >> .claude/sessions/current/STATUS.md << EOF
### Update: $(date +%H:%M:%S)
- Completed: [what was done]
- Modified: [files changed]
- Next: [immediate next step]
EOF
```

## 🛠️ Initial Project Setup

```bash
# QUICKSTART: Copy and run the setup script
# 1. Copy content from "Claude Code Initial Setup Script" artifact
# 2. Save as setup_claude.sh in your project root
# 3. Run:
chmod +x setup_claude.sh
./setup_claude.sh

# This automatically creates:
# - All required directories
# - All automation scripts
# - Git hooks
# - Basic Doxyfile
# - Initial documentation

# OR manual setup:

# 1. Create script directories
mkdir -p scripts .claude/scripts

# 2. Generate Doxyfile (see Doxygen Setup artifact for config)
doxygen -g Doxyfile

# 3. Create documentation generation script
# Copy scripts/generate_docs.sh from Doxygen Setup artifact
chmod +x scripts/generate_docs.sh

# 4. Create session start script  
# Copy .claude/scripts/start_session.sh from Doxygen Setup artifact
chmod +x .claude/scripts/start_session.sh

# 5. Create session archive script
# Copy .claude/scripts/archive_session.sh from Archive Session Script artifact  
chmod +x .claude/scripts/archive_session.sh

# 6. Optional: Create Python doc extractor
# Copy scripts/extract_docs.py from Doxygen Setup artifact

# 7. Optional: Install git pre-commit hook
# Copy .git/hooks/pre-commit from Doxygen Setup artifact
chmod +x .git/hooks/pre-commit

# 8. Test setup
scripts/generate_docs.sh
```

## 🛠️ PlatformIO Commands

```bash
# Build
pio run -e <env>                    # Build specific environment
pio run -t clean                    # Clean build

# Test  
pio test -e <env> -f <test_name>    # Run specific test
pio test --list-targets             # List available targets

# Device (Ask permission first)
pio device list                     # List connected devices
pio run -t upload -e <env>          # Upload firmware
pio device monitor -b 115200        # Monitor serial (external tool)

# Libraries
pio pkg list                        # List dependencies
pio pkg update                      # Update packages
```

## 📁 Project Structure

```
.
├── .claude/                       # Claude session data
│   ├── sessions/                  # All session data
│   │   ├── current/              # Symlink to active session
│   │   ├── 20250120_143022/     # Session archives
│   │   │   ├── STATUS.md        # Session status tracking
│   │   │   ├── PLAN.md          # Session planning
│   │   │   └── DECISIONS.md     # Technical decisions
│   │   └── index.md              # Session index
│   ├── scripts/                  # Claude automation scripts
│   │   ├── start_session.sh      # Session initialization
│   │   └── archive_session.sh    # Session archiving
│   └── templates/                # Reusable templates
├── scripts/                      # Project automation
│   ├── generate_docs.sh          # Doxygen with statistics  
│   └── extract_docs.py           # Parse documentation
├── .git/hooks/                   # Git automation
│   └── pre-commit                # Auto-generate docs on commit
├── docs/                         
│   ├── doxygen/                  # Generated documentation
│   │   ├── html/                 # Browse this FIRST
│   │   └── xml/                  # For parsing
│   └── README.md                 # Project documentation
├── src/                          # Source files
├── test/                         # Unit tests
├── Doxyfile                      # Doxygen configuration
└── platformio.ini                # PlatformIO config
```

## 🚫 Common Pitfalls to Avoid

1. **DON'T explore entire codebase** - Use `scripts/extract_docs.py` instead
2. **DON'T rewrite plans** - Append progress to existing docs
3. **DON'T skip baseline commits** - Always commit before changes
4. **DON'T forget Doxygen** - Use `scripts/generate_docs.sh` after changes
5. **DON'T use TODO placeholders** - Complete each change fully
6. **DON'T read source first** - Check `docs/doxygen/html/` documentation
7. **DON'T manually track sessions** - Use `.claude/scripts/start_session.sh`

## 💾 Session Continuity

**At session end, ALWAYS:**
```bash
# 1. Final documentation generation with statistics
scripts/generate_docs.sh

# 2. Session summary
cat >> .claude/sessions/current/STATUS.md << EOF

## Session End: $(date)
### Completed:
$(git log --oneline -n 5 --since="4 hours ago")

### Documentation Updates:
$(scripts/generate_docs.sh | grep "HTML files:" | awk '{print $3}') files regenerated

### Next Session Should:
- [Specific next steps]
EOF

# 3. Archive session (automated)
.claude/scripts/archive_session.sh
# OR manually:
SESSION_ARCHIVE=.claude/sessions/$(date +%Y%m%d_%H%M%S)_completed
cp -r .claude/sessions/current $SESSION_ARCHIVE

# 4. Final commit
git add -A && git commit -m "session: completed $(date +%Y%m%d_%H%M%S)"
```

## 🔍 Quick Debug Checks

```bash
# Verify environment
git branch --show-current              # Should be 'claude-branch'
git status --short                     # Check uncommitted changes
ls -la .claude/sessions/current/       # Verify session exists

# Documentation health  
scripts/generate_docs.sh | tail -5        # See statistics
grep -c "warning:" docs/doxygen/doxygen.log    # Check warnings

# Build status
pio run -e <env> --silent && echo "✓ Build OK" || echo "✗ Build Failed"
pio run -e <env> -t size              # Memory usage

# Recent work
git log --oneline -5                  # Recent commits
tail -20 .claude/sessions/current/STATUS.md    # Session progress
```

### Scripts Overview

**Core automation scripts (create these from Doxygen Setup artifact):**

1. **`scripts/generate_docs.sh`**
   - Generates Doxygen documentation
   - Shows statistics (file count, classes, warnings)
   - Opens in browser automatically

2. **`scripts/extract_docs.py`**
   - Parses Doxygen HTML output
   - Lists all classes and their methods
   - Enables quick searching without reading source

3. **`.claude/scripts/start_session.sh`**
   - Creates timestamped session directory
   - Generates initial documentation
   - Creates DOC_SUMMARY.md with project overview
   - Sets up session symlinks

4. **`.claude/scripts/archive_session.sh`** (simple version):
   ```bash
   #!/bin/bash
   # Archive completed session
   ARCHIVE=.claude/sessions/$(date +%Y%m%d_%H%M%S)_completed
   cp -r .claude/sessions/current $ARCHIVE
   echo "Session archived to: $ARCHIVE"
   ```

5. **`.git/hooks/pre-commit`**
   - Auto-generates docs when source files change
   - Adds documentation to commit automatically

## 📊 Documentation-First Token Optimization

1. **ALWAYS read Doxygen HTML before source files**
2. **Use documentation search instead of code grep**
3. **Reference classes/methods from docs, not file paths**
4. **Use `/clear` after completing each implementation step**
5. **Limit source file reading to missing documentation only**

## 🎯 Task Template

When starting a new task, user provides:
```markdown
## Task: [Name/Description]
## Type: [ ] Bug Fix  [ ] Feature  [ ] Refactor  [ ] Enhancement
## Problem: [What's wrong or what's needed]
## Context: [Any background information]
## Hardware: [Board/sensors involved if relevant]
## Constraints: [Timing/memory/power limits if any]
## Success Criteria: [How to verify completion]
## Keywords: [Terms to search in documentation]
```

**Claude will investigate and find relevant files using:**
1. Doxygen documentation search
2. Class/function dependency graphs
3. Keyword searching in generated docs
4. Only reading source when docs insufficient

## ⚡ Emergency Recovery

If session is interrupted:
```bash
# 1. Check last good state
git log --oneline -5
git status

# 2. Restore if needed
git checkout -- .  # Discard all changes
# OR
git stash          # Save changes for later

# 3. Regenerate documentation with stats
scripts/generate_docs.sh

# 4. Resume from session status
cat .claude/sessions/current/STATUS.md
cat .claude/sessions/current/DOC_SUMMARY.md  # If using start_session.sh

# 5. View recent changes in docs
python3 scripts/extract_docs.py  # See all classes/methods
# OR manually
find docs/doxygen/html -newer .claude/sessions/current/STATUS.md -name "*.html" | head -10
```

## 🚀 Quick Reference Card

```bash
# Start session (automated)
.claude/scripts/start_session.sh

# Or manual start
scripts/generate_docs.sh && git add -A && git commit -m "baseline: $(date +%H%M%S)"

# Search docs (not code!)
grep -r "keyword" docs/doxygen/html/
# Or use Python extractor
python3 scripts/extract_docs.py | grep "keyword"

# After changes
scripts/generate_docs.sh && git add -A && git commit -m "feat: description"

# End session  
.claude/scripts/archive_session.sh
# Or manually
cp -r .claude/sessions/current .claude/sessions/$(date +%Y%m%d_%H%M%S)_completed
```

---
**Remember:** Documentation-first approach saves tokens and improves accuracy. Always regenerate Doxygen after changes!

**Available Artifacts for Setup:**
1. **"Enhanced CLAUDE.md"** - This file
2. **"Doxygen Setup and Documentation Generation"** - Complete Doxygen configuration and scripts
3. **"Archive Session Script"** - Session archiving automation
4. **"Claude Code Initial Setup Script"** - One-command setup for all scripts

**Key Scripts Created by Setup:**
- `scripts/generate_docs.sh` - Generates docs with statistics
- `scripts/extract_docs.py` - Parses docs for class/method info  
- `.claude/scripts/start_session.sh` - Automated session initialization
- `.claude/scripts/archive_session.sh` - Session archiving with summary
- `.git/hooks/pre-commit` - Auto-generates docs on commit