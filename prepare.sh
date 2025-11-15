#!/usr/bin/env bash
# Prepare the workspace for building the project and running the Python tools.
# Usage:
#   bash prepare.sh            # create directories only
#   bash prepare.sh --venv     # also create a Python venv and install requirements

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"

WORKLOADS_DIR="$ROOT_DIR/workloads"
BUILD_DIR="$ROOT_DIR/build"
VENV_DIR="$ROOT_DIR/.venv"

show_help() {
  cat <<-EOF
Usage: $0 [--venv]

Creates basic directories required by the project:
  - $WORKLOADS_DIR
  - $BUILD_DIR

Options:
  --venv    Create a Python virtual environment at .venv and install packages from requirements.txt
  --help    Show this help message
EOF
}

if [[ ${1:-} == "--help" || ${1:-} == "-h" ]]; then
  show_help
  exit 0
fi

echo "Preparing workspace in: $ROOT_DIR"

mkdir -p "$WORKLOADS_DIR"
mkdir -p "$BUILD_DIR"

echo "Created directories:"
echo "  - $WORKLOADS_DIR"
echo "  - $BUILD_DIR"

if [[ ${1:-} == "--venv" ]]; then
  if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found in PATH. Please install Python 3 before creating a venv." >&2
    exit 1
  fi

  echo "Creating virtual environment at $VENV_DIR"
  python3 -m venv "$VENV_DIR"
  # Activate and upgrade pip, then install requirements
  # shellcheck disable=SC1090
  source "$VENV_DIR/bin/activate"
  pip install --upgrade pip setuptools wheel
  if [[ -f "$ROOT_DIR/requirements.txt" ]]; then
    echo "Installing Python requirements from requirements.txt"
    pip install -r "$ROOT_DIR/requirements.txt"
  else
    echo "No requirements.txt found in $ROOT_DIR; skipping pip install"
  fi
  deactivate
  echo "Virtual environment created (activate with: source $VENV_DIR/bin/activate)"
fi

echo "Preparation complete. You can now run CMake from the $BUILD_DIR directory, e.g."
echo "  cd $BUILD_DIR && cmake .. && make"
