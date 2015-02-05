set TESTDATA_DIR=%1

del multipolygon.db multipolygon-tests.json
%2\testdata-multipolygon %TESTDATA_DIR%\grid\data\all.osm >multipolygon.log 2>&1
if ERRORLEVEL 1 (exit /b 1)
ruby %TESTDATA_DIR%\bin\compare-areas.rb %TESTDATA_DIR%\grid\data\tests.json multipolygon-tests.json
if ERRORLEVEL 1 (exit /b 1)
