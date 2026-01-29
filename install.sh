#!/usr/bin/env bash
set -euo pipefail

LLVM_VER="${LLVM_VER:-20}"

# --- deps
sudo apt-get update
sudo apt-get install -y wget lsb-release ca-certificates gnupg

# --- add apt.llvm.org repo (recommended: /etc/apt/keyrings + signed-by)
UBUNTU_CODENAME="$(lsb_release -cs)"
KEYRING="/etc/apt/keyrings/apt.llvm.org.gpg"
LIST_FILE="/etc/apt/sources.list.d/llvm.list"

sudo mkdir -p /etc/apt/keyrings

# Import key only if missing
if [[ ! -f "$KEYRING" ]]; then
  wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key \
    | gpg --dearmor \
    | sudo tee "$KEYRING" >/dev/null
fi

# Ensure repo line exists (idempotent)
REPO_LINE="deb [signed-by=${KEYRING}] http://apt.llvm.org/${UBUNTU_CODENAME}/ llvm-toolchain-${UBUNTU_CODENAME}-${LLVM_VER} main"
if [[ ! -f "$LIST_FILE" ]] || ! grep -q "llvm-toolchain-${UBUNTU_CODENAME}-${LLVM_VER}" "$LIST_FILE"; then
  echo "$REPO_LINE" | sudo tee "$LIST_FILE" >/dev/null
fi

sudo apt-get update

# --- install toolchain
sudo apt-get install -y "clang-${LLVM_VER}" "clang++-${LLVM_VER}" "lld-${LLVM_VER}" "llvm-${LLVM_VER}"

# --- point defaults to this version (safe to repeat)
sudo update-alternatives --install /usr/bin/clang clang "/usr/bin/clang-${LLVM_VER}" 100
sudo update-alternatives --install /usr/bin/clang++ clang++ "/usr/bin/clang++-${LLVM_VER}" 100
sudo update-alternatives --install /usr/bin/lld lld "/usr/bin/lld-${LLVM_VER}" 100
sudo update-alternatives --install /usr/bin/llvm-objdump llvm-objdump "/usr/bin/llvm-objdump-${LLVM_VER}" 100
sudo update-alternatives --install /usr/bin/llvm-objcopy llvm-objcopy "/usr/bin/llvm-objcopy-${LLVM_VER}" 100
sudo update-alternatives --install /usr/bin/llvm-strings llvm-strings "/usr/bin/llvm-strings-${LLVM_VER}" 100

sudo update-alternatives --set clang "/usr/bin/clang-${LLVM_VER}"
sudo update-alternatives --set clang++ "/usr/bin/clang++-${LLVM_VER}"
sudo update-alternatives --set lld "/usr/bin/lld-${LLVM_VER}"
sudo update-alternatives --set llvm-objdump "/usr/bin/llvm-objdump-${LLVM_VER}"
sudo update-alternatives --set llvm-objcopy "/usr/bin/llvm-objcopy-${LLVM_VER}"
sudo update-alternatives --set llvm-strings "/usr/bin/llvm-strings-${LLVM_VER}"
