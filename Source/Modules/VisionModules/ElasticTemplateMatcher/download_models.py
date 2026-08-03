import os
from pathlib import Path

repository_root = Path(__file__).resolve().parents[4]
os.environ.setdefault("TORCH_HOME", str(repository_root / ".model-cache"))

from lightglue import ALIKED, LightGlue

ALIKED(max_num_keypoints=512).eval()
LightGlue(features="aliked").eval()
print("ALIKED and LightGlue weights are ready.")
