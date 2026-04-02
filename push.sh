#!/bin/bash
# Push Microcosm VST to GitHub

cd /root/.openclaw/workspace/microcosm-vst

# Initialize git if needed
if [ ! -d .git ]; then
    git init
fi

# Configure (optional, git will use defaults)
git config user.email "kuuhaku@openclaw.ai"
git config user.name "Kuuhaku"

# Add all files
git add -A

# Commit
git commit -m "Phase 2: Modular architecture with Helix-style processing chain

- Converted GranularEngine to ProcessingModule
- Added 5 Activity Modes: Chop, Stretch, Reverse, Scatter, Glitch
- Added 5 Effects: Reverb, Delay, Filter, Chorus, Bitcrusher
- Modular chain allows adding/removing/reordering modules
- UI shows visual chain with bypass toggles
- Each module has independent parameters"