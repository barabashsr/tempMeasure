#!/bin/bash
# Generate documentation with statistics

echo "Generating Doxygen documentation..."

# Clean previous docs
rm -rf docs/doxygen/html docs/doxygen/xml

# Generate docs
doxygen Doxyfile 2>&1 | tee docs/doxygen/doxygen.log

# Count results
echo ""
echo "Documentation Statistics:"
echo "- HTML files: $(find docs/doxygen/html -name "*.html" | wc -l)"
echo "- Classes documented: $(grep -c "class=\"el\"" docs/doxygen/html/annotated.html 2>/dev/null || echo 0)"
echo "- Files documented: $(grep -c "class=\"el\"" docs/doxygen/html/files.html 2>/dev/null || echo 0)"

# Check for warnings
WARNINGS=$(grep -c "warning:" docs/doxygen/doxygen.log)
if [ $WARNINGS -gt 0 ]; then
    echo "- Warnings: $WARNINGS (see docs/doxygen/doxygen.log)"
else
    echo "- Warnings: None"
fi

# Open in browser (optional)
if command -v xdg-open &> /dev/null; then
    xdg-open docs/doxygen/html/index.html
elif command -v open &> /dev/null; then
    open docs/doxygen/html/index.html
fi