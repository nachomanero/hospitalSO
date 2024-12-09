Para ejecutar el proyecto ejecutar los siguientes comandos (Windows):
    1. Abrir wsl (en caso de no tenerlo instalado: wsl --install)
    2. Ejecutar: 
        sudo apt update
        sudo apt install build-essential
    3. Navegar al directorio del proyecto y compilar: 
        gcc -o hospital hospital.c -lpthread -lrt
    4. Ejecutar el ejecutable:
        .\hospital
        

