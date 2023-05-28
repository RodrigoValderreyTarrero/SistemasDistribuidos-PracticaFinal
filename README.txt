INDICE DEL README:
1. PREPARACIÓN DEL ENTORNO
2. COMPILACIÓN DEL PROYECTO
3. EJECUCIÓN


1. PREPARACIÓN DEL ENTORNO:

En primera instancia se deben abrir tantas terminales como números de clientes se deseen más 2.
Estas dos terminales extras sirven:
    1. Una para el servidor web, que debe estar ejecutandose durante la ejecución de todo el programa.
    2. EL servidor, que también es obligatorio que esté ejecutandose en todo momento.

Para que el entorno funcione correctamente debe especificarse en todas las terminales que se quieran utilizar
para clientes el siguiente comando: "export ip_servidor_web=localhost" o en su defecto la IP que se le quiera asignar al
servicio web.

2. COMPILACIÓN DEL PROYECTO:

Para compilar el proyecto, se debe ejecutar el comando "make". Este generará el ejecutable del servidor "server".
La parte de pyhton no debe ser compilada por tratarse de un lenguaje interpretado.

3. EJECUCIÓN:

    -Terminal del servidor:
        Se ejecutará: "./server -p <puerto>" donde puerto puede ser el deseado (preferiblemente alto para evitar los reservados).
        Ejemplo: ./server -p 8888

    -Terminal del servidor web:
        Se debe ejecutar el comando: "python3 ./servidor_web.py"

    -Terminal de cada cliente:
        Se recuerda que debe establecer la variable de entorno export ip_servidor_web=<ip>
        Donde la ip será la que se quiera dar al servidor web. El grupo empleó tanto localhost como
        la dirección IP asignada a nuestros dispositivos en nuestra red local. Aunque ya se ha avisado se recuerda ya que
        si no se hace el servidor web no sirve.

        Ejecutar: "./python3 ./client.py -s <ip> -p <puerto>"
        Donde la IP será la del servidor y el puerto al que se debe conectar.






