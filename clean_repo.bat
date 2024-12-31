@echo off
echo ==================================
echo    Clean Repository Script
echo ==================================
echo.

echo Fetching latest changes...
git fetch origin main

echo Pulling latest changes...
git pull origin main

echo Adding .gitignore...
git add .gitignore

echo Removing build directory from Git tracking...
git rm -r --cached build

echo Committing changes...
git commit -m "Remove build directory from Git tracking and add .gitignore"

echo Pushing changes...
git push origin main

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Error: Failed to push changes.
    echo You might need to resolve conflicts manually.
    echo Try these steps:
    echo 1. git pull origin main
    echo 2. Resolve any conflicts
    echo 3. git add .
    echo 4. git commit -m "Merge and remove build directory"
    echo 5. git push origin main
    echo.
)

echo.
echo Done! The build directory has been removed from Git tracking.
echo It will remain on your local machine but won't be tracked by Git anymore.
echo.
pause
