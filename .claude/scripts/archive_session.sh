#!/bin/bash
# Archive Claude Code session with metadata
# Location: .claude/scripts/archive_session.sh

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Archiving current session...${NC}"

# Get current session info
if [ ! -d ".claude/sessions/current" ]; then
    echo "Error: No current session found!"
    exit 1
fi

# Generate final documentation
if [ -f "scripts/generate_docs.sh" ]; then
    echo "Generating final documentation..."
    scripts/generate_docs.sh > /dev/null 2>&1
fi

# Create archive name with completion timestamp
ARCHIVE_NAME=".claude/sessions/$(date +%Y%m%d_%H%M%S)_completed"

# Add final session summary
cat >> .claude/sessions/current/STATUS.md << EOF

## Session Archived: $(date)
### Final Statistics:
- Commits this session: $(git log --oneline --since="8 hours ago" | wc -l)
- Files modified: $(git diff --name-only $(git log --format="%H" -n 1 --skip=10)..HEAD | wc -l)
- Documentation files: $(find docs/doxygen/html -name "*.html" 2>/dev/null | wc -l)
EOF

# Copy current session to archive
cp -r .claude/sessions/current "$ARCHIVE_NAME"

# Create session index entry
cat >> .claude/sessions/index.md << EOF

## $(basename "$ARCHIVE_NAME")
- Task: $(grep -m1 "Task:" .claude/sessions/current/STATUS.md | cut -d: -f2-)
- Duration: $(grep "Started:" .claude/sessions/current/STATUS.md | cut -d: -f2-) to $(date)
- Commits: $(git log --oneline --since="8 hours ago" | wc -l)
EOF

# Summary
echo -e "${GREEN}✓ Session archived to: $ARCHIVE_NAME${NC}"
echo -e "${GREEN}Session Summary:${NC}"
tail -15 "$ARCHIVE_NAME/STATUS.md" | grep -E "(Completed:|Next Session Should:)" -A 3

# Offer to clear current session
echo -e "\n${YELLOW}Clear current session for next time? (y/n)${NC}"
read -r response
if [[ "$response" =~ ^[Yy]$ ]]; then
    rm -rf .claude/sessions/current
    echo -e "${GREEN}✓ Current session cleared${NC}"
else
    echo "Current session preserved"
fi