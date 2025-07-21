### Workflow Reminder 
- **ALWAYS start with reading CLAUDE.md file**

# ⚠️ CRITICAL: MANDATORY WORKFLOW - READ FIRST ⚠️

**THIS FILE CONTAINS MANDATORY INSTRUCTIONS FOR ALL AI ASSISTANTS**

🔴 **STOP**: Do not proceed with ANY task until you have:
1. ✅ Read this entire file
2. ✅ Created a baseline commit 
3. ✅ Started a session using the documented scripts
4. ✅ Generated current documentation

**Ignoring these instructions violates project requirements and will result in rejected work.**

---

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
5. **ALWAYS ask questions during the planning stage**

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

[Rest of the file remains unchanged]