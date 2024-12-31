@echo off
echo ==================================
echo    Fix Repository Script
echo ==================================
echo.

echo Cleaning up any unfinished merges...
git merge --abort

echo Resetting local changes...
git reset --mixed

echo Adding .gitignore...
git add .gitignore

echo Committing .gitignore...
git commit -m "Add .gitignore to exclude build files"

echo Removing build directory from remote...
git push origin --delete build 2>nul

echo Fetching latest changes...
git fetch origin

echo Checking out main branch...
git checkout main

echo Pulling latest changes...
git pull origin main

echo Force removing build directory from tracking...
git rm -r --cached build 2>nul
git rm -r --cached */build 2>nul

echo Committing changes...
git add .
git commit -m "Clean up repository and remove build directories"

echo Pushing changes...
git push origin main

echo.
echo Repository cleanup completed!
echo If the build directory is still visible on GitHub:
echo 1. Go to your repository settings on GitHub
echo 2. Delete the repository
echo 3. Create a new repository
echo 4. Push your clean local copy
echo.
pause
