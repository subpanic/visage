# Visage Documentation

## Building locally

```
pip install -r requirements.txt
doxygen Doxyfile
sphinx-build -b html . _build/html
```

## Structure

- `conf.py`: Sphinx configuration (MyST Markdown + Breathe).
- `Doxyfile`: Doxygen configuration (XML output for Breathe).
- `requirements.txt`: Python doc dependencies.
- `index.md` and guides: narrative docs and examples.
- `api/index.rst`: entry point for the generated API reference.

## Notes

- Doxygen XML must exist before Sphinx can render API pages.
- Outputs are written to `_build/` (ignored by git).
- The CMake `docs` target will run Doxygen then Sphinx once enabled.
