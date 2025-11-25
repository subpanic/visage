import os
from datetime import datetime

project = "Visage"
author = "Visage Developers"
year = str(datetime.now().year)
copyright = f"{year}, {author}"

extensions = [
  "myst_parser",
  "breathe",
  "sphinx.ext.autosectionlabel",
  "sphinx.ext.todo",
  "sphinx.ext.intersphinx",
  "sphinx_copybutton",
]

myst_enable_extensions = [
  "colon_fence",
  "deflist",
  "fieldlist",
]
myst_heading_anchors = 3

autosectionlabel_prefix_document = True
todo_include_todos = True

templates_path = ["_templates"]
exclude_patterns = ["_build"]
source_suffix = {
  ".md": "markdown",
  ".rst": "restructuredtext",
}

html_theme = "furo"
html_title = "Visage Documentation"
html_static_path = ["_static"]

_here = os.path.abspath(os.path.dirname(__file__))
_doxygen_xml = os.path.join(_here, "_build", "doxygen", "xml")
breathe_projects = { "visage": _doxygen_xml }
breathe_default_project = "visage"
breathe_default_members = ("members", "undoc-members")

intersphinx_mapping = {
  "python": ("https://docs.python.org/3", None),
}
