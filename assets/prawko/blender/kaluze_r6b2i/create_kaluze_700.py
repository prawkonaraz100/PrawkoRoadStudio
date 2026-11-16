# SPDX-License-Identifier: Apache-2.0
"""R6b2i curbside rainwater; reuse R6b2h mesh/material pipeline, +Z, metres."""
from pathlib import Path
import sys
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent/'kaluze_r6b2h'))
import create_kaluze as src
src.ROOT=HERE
src.export('r6b2i-kaluza-przykraweznikowa-700',
           'Kałuża przy krawężniku 7,0 m',62026,3.0,.93)
print('R6B2I_REUSED_PRAWKOWATER_MATERIAL_AND_GLB_PIPELINE_PASS')
