# Workspace-local ESP-IDF environment. Dot-source before idf.py:
#   . .\scripts\idf_env_workspace.ps1
#
# The right cluster needs ESP-IDF >= 5.4 (the esp_lcd_st7701 >= 2.0.0 panel
# driver requires it). These point at the 5.4.2 install on this PC and its
# working Python 3.11 venv. Set unconditionally so a shell profile that pre-sets
# these to another (e.g. 5.3.x) install can't break activation. Edit the paths
# here if your ESP-IDF 5.4+ install lives elsewhere.
$env:IDF_PATH            = "C:\esp\v5.4.2\v5.4.2\esp-idf"
$env:IDF_TOOLS_PATH      = "C:\esp\v5.4.2\v5.4.2\tools"
$env:IDF_PYTHON_ENV_PATH = "C:\esp\v5.4.2\v5.4.2\tools\python"

# A system Python (e.g. 3.14) on PATH makes export.ps1 look for a venv name that
# does not exist. Setting IDF_PYTHON_ENV_PATH above + clearing these avoids that.
$env:PYTHONPATH = $null
$env:PYTHONHOME = $null

# Activate ESP-IDF: adds the xtensa toolchain, cmake and ninja to PATH and
# validates the Python venv. Relax the caller's ErrorActionPreference around it
# so export.ps1's non-terminating diagnostics don't abort the build scripts.
$prevEAP = $ErrorActionPreference
$ErrorActionPreference = "Continue"
. "$env:IDF_PATH\export.ps1" | Out-Null
$ErrorActionPreference = $prevEAP
