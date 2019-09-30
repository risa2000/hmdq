PUSHD %CD%
cd %~dp0
hmdq.exe -n --out_json hmdq_data.json
POPD
