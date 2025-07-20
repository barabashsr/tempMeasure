#!/bin/bash
# Start a new Claude session with fresh docs

# Generate docs
echo "📚 Generating documentation..."
doxygen Doxyfile > /dev/null 2>&1

# Create session
SESSION_ID=$(date +%Y%m%d_%H%M%S)
mkdir -p .claude/sessions/$SESSION_ID
ln -sfn .claude/sessions/$SESSION_ID .claude/sessions/current

# Create summary
cat > .claude/sessions/current/DOC_SUMMARY.md << EOF
# Documentation Summary
Generated: $(date)

## Classes Found
$(grep -c "class=\"el\"" docs/doxygen/html/annotated.html || echo 0) classes documented

## Key Modules
$(ls src/*.cpp 2>/dev/null | head -5)

## Search Index
Ready at: docs/doxygen/html/search/
EOF

echo "✅ Session $SESSION_ID ready!"
echo "📖 Documentation available at: docs/doxygen/html/index.html"