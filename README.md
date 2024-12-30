# Chunkbiomes

This is a *very limited* port of some of [Chunkbase](https://www.chunkbase.com/apps/seed-map)'s Bedrock search filters to standalone [Cubiomes](https://www.github.com/Cubitect/cubiomes)-esque functions. This way, they can be used in C/C++ programs to check thousands of seeds per second for the features, which is otherwise impossible to do with the website alone.

Programs are compiled by
- installing `gcc` and `makefile`,
- using `makefile` to create a static archive of Cubiomes (follow that repository's instructions), and
- typing `gcc "path/to/your/program" "path/to/Bfinders.c" "path/to/the/Cubiomes/static/archive" -o "desiredProgramExecutableName"` in your terminal of choice.

Logic differentiating between Bastion Remnants and Fortresses has not been added yet.

Thank you to Fragrant_Result_186 for calculating the salt, region size, and separation of Ancient Cities."# ChunkBiomesGUI" 
