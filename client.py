import subprocess
import sys
import PySimpleGUI as sg
from enum import Enum
import argparse
import socket
import struct
import threading
from suds.client import Client
import os

cortar_hilo = False
#Dado que el hilo de escucha solo lee este dato pero no lo modifica no hay riesgo de condiciones de carrera ya que solo
#un hilo podrá modificar su valor


class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _quit = 0
    _username = None
    _alias = None
    _date = None

    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user, window):
        """Gracias a esta función se borra a un usuario de la BBDD"""
        HOST = client._server  # La dirección IP del servidor
        PORT = client._port        # El puerto que utiliza el servidor
        #Emviamos el mensaje e imprimimsos la información asociada al error
        n = enviar_mensaje(window, ["UNREGISTER",client._alias],HOST, PORT)
        if int(n) == 0:
            window['_SERVER_'].print("s> UNREGISTER OK")
            return client.RC.OK
        else:
            if n == 1:
                window['_SERVER_'].print("s> USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            else:
                window['_SERVER_'].print("s> UNREGISTER FAIL")
                return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 *
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    @staticmethod
    def  register(user, window):
        """Función que permite ingresar en la BBDD a un cliente"""
        HOST = client._server  # La dirección IP del servidor
        PORT = client._port        # El puerto que utiliza el servidor
        #Declaramos la lista que enviaremos
        lista_mensajes = ["REGISTER",client._username, client._alias, client._date]
        #Enviamos los datos y capuramos el error
        n = enviar_mensaje(window, lista_mensajes ,HOST, PORT, False, False)
        #Imprimimos en función del error capturado
        if n == 0:
            window['_SERVER_'].print("s> REGISTER OK")
            return client.RC.OK
        else:
            client._alias = None;
            #Para prevenir que luego haga un conect habiendo habido un error.
            if n == 1:
                window['_SERVER_'].print("s> USERNAME IN USE")
                return client.RC.USER_ERROR
            else:
                window['_SERVER_'].print("s> REGISTER FAIL")
                return client.RC.ERROR

    # *
    # * @param user - User name to connect to the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  connect(user, window):
        """Cambiamos el estado del usuario a conectado"""
        #Obtenemos la variable global para decidir si hay que cortar o no el hilo
        global cortar_hilo
        cortar_hilo = False
        HOST = client._server  # La dirección IP del servidor
        PORT = client._port        # El puerto que utiliza el servidor
        #Creamos el socket de escucha
        socket_mensajes = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        socket_mensajes.bind((str('0.0.0.0') , 0))
        #Lanzamos el hilo
        hilo = threading.Thread(target=escuchar_servidor, args=(window, socket_mensajes,))
        hilo.start()
        port = str(socket_mensajes.getsockname()[1])
        #Enviamos el mensaje
        n = enviar_mensaje(window, ["CONNECT",client._alias, port],HOST, port)
        #Se actua en función del error
        if n == 0:
            window['_SERVER_'].print("s> CONNECT OK")
            return client.RC.OK
        else:
            if n == 1:
                cortar_hilo = True
                window['_SERVER_'].print("s> CONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif n == 2:
                cortar_hilo = False
                window['_SERVER_'].print("s> USER ALREADY CONNECTED")
                return client.RC.USER_ERROR
            else:
                cortar_hilo = True
                window['_SERVER_'].print("s> CONNECT FAIL")
                return client.RC.ERROR

    # *
    # * @param user - User name to disconnect from the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def  disconnect(user, window):
        """Modifica el valor de conectado o desconectado"""
        global cortar_hilo
        #Cortar hilo nos ayuda a saber si el de escucha debe acabar
        HOST = client._server  # La dirección IP del servidor
        PORT = client._port        # El puerto que utiliza el servidor
        #Enviamos la información y capturamos el error
        n = enviar_mensaje(window, ["DISCONNECT",client._alias],HOST, PORT)
        #Actuamos en función del error
        if int(n) == 0:
            cortar_hilo = True
            window['_SERVER_'].print("s> DISCONNECT OK")
            return client.RC.OK
        else:
            if int(n) == 1:
                window['_SERVER_'].print("s> DISCONNECT FAIL / USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif int(n) == 2:
                window['_SERVER_'].print("s> DISCONNECT FAIL / USER NOT CONNECTED")
                return client.RC.USER_ERROR
            else:
                window['_SERVER_'].print("s> DISCONNECT FAIL")
                return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  send(user, message, window):
        """EL send permite enviar un usuario especídifco a un usuario especificado"""

        HOST = client._server  # La dirección IP del servidor
        PORT = client._port        # El puerto que utiliza el servidor
        #Capturamos el error
        n,id = enviar_mensaje(window, ["SEND",client._alias, user, message],HOST, PORT, True)

        #SI TOD0 FUE CORRECTO, actuamos imprimiendo lo que corresponda
        if n == 0:
            try:
                window['_SERVER_'].print("s> SEND OK - MESSAGE " + str(int(id)))
                return client.RC.OK
            except:
                window['_SERVER_'].print("s> SEND FAIL")
                return client.RC.ERROR
        elif n == 1:
            window['_SERVER_'].print("s> SEND FAIL / USER DOES NOT EXIST")
            return client.RC.USER_ERROR
        else:
            window['_SERVER_'].print("s> SEND FAIL")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * @param file    - file  to be sent

    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user, message, file, window):
        """Ha sido suprimida por el profesorado"""
        window['_SERVER_'].print("s> SENDATTACH MESSAGE OK")
        print("SEND ATTACH " + user + " " + message + " " + file)
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  connectedUsers(window):
        """Connectusers nos permite conocer que usaurios están en estado conectado y cuantos son"""

        HOST = client._server  # La dirección IP del servidor
        PORT = client._port        # El puerto que utiliza el servidor
        #Enviamos el mensaje y capturamos la información esperaada
        n = enviar_mensaje(window, ["CONNECTEDUSERS",client._alias],HOST, PORT, False, True)
        if n == 0:
            window['_CLIENT_'].print("c> CONNECTEDUSERS")
        if n == 1:
            window['_CLIENT_'].print("c> CONNECTED USERS")

        return client.RC.ERROR


    @staticmethod
    def window_register():
        layout_register = [[sg.Text('Ful Name:'),sg.Input('Text',key='_REGISTERNAME_', do_not_clear=True, expand_x=True)],
                           [sg.Text('Alias:'),sg.Input('Text',key='_REGISTERALIAS_', do_not_clear=True, expand_x=True)],
                           [sg.Text('Date of birth:'),sg.Input('',key='_REGISTERDATE_', do_not_clear=True, expand_x=True, disabled=True, use_readonly_for_disable=False),
                            sg.CalendarButton("Select Date",close_when_date_chosen=True, target="_REGISTERDATE_", format='%d-%m-%Y',size=(10,1))],
                           [sg.Button('SUBMIT', button_color=('white', 'blue'))]
                           ]

        layout = [[sg.Column(layout_register, element_justification='center', expand_x=True, expand_y=True)]]

        window = sg.Window("REGISTER USER", layout, modal=True)
        choice = None

        while True:
            event, values = window.read()

            if (event in (sg.WINDOW_CLOSED, "-ESCAPE-")):
                break

            if event == "SUBMIT":
                if(values['_REGISTERNAME_'] == 'Text' or values['_REGISTERNAME_'] == '' or values['_REGISTERALIAS_'] == 'Text' or values['_REGISTERALIAS_'] == '' or values['_REGISTERDATE_'] == ''):
                    sg.Popup('Registration error', title='Please fill in the fields to register.', button_type=5, auto_close=True, auto_close_duration=1)
                    continue

                client._username = values['_REGISTERNAME_']
                client._alias = values['_REGISTERALIAS_']
                client._date = values['_REGISTERDATE_']
                break
        window.Close()


    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;

        client._server = args.s
        client._port = args.p

        return True


    def main(argv):

        if (not client.parseArguments(argv)):
            client.usage()
            exit()

        lay_col = [[sg.Button('REGISTER',expand_x=True, expand_y=True),
                    sg.Button('UNREGISTER',expand_x=True, expand_y=True),
                    sg.Button('CONNECT',expand_x=True, expand_y=True),
                    sg.Button('DISCONNECT',expand_x=True, expand_y=True),
                    sg.Button('CONNECTED USERS',expand_x=True, expand_y=True)],
                   [sg.Text('Dest:'),sg.Input('User',key='_INDEST_', do_not_clear=True, expand_x=True),
                    sg.Text('Message:'),sg.Input('Text',key='_IN_', do_not_clear=True, expand_x=True),
                    sg.Button('SEND',expand_x=True, expand_y=False)],
                   [sg.Text('Attached File:'), sg.In(key='_FILE_', do_not_clear=True, expand_x=True), sg.FileBrowse(),
                    sg.Button('SENDATTACH',expand_x=True, expand_y=False)],
                   [sg.Multiline(key='_CLIENT_', disabled=True, autoscroll=True, size=(60,15), expand_x=True, expand_y=True),
                    sg.Multiline(key='_SERVER_', disabled=True, autoscroll=True, size=(60,15), expand_x=True, expand_y=True)],
                   [sg.Button('QUIT', button_color=('white', 'red'))]
                   ]


        layout = [[sg.Column(lay_col, element_justification='center', expand_x=True, expand_y=True)]]

        window = sg.Window('Messenger', layout, resizable=True, finalize=True, size=(1000,400))
        window.bind("<Escape>", "-ESCAPE-")


        while True:
            event, values = window.Read()

            if (event in (None, 'QUIT')) or (event in (sg.WINDOW_CLOSED, "-ESCAPE-")):
                sg.Popup('Closing Client APP', title='Closing', button_type=5, auto_close=True, auto_close_duration=1)
                break

            #if (values['_IN_'] == '') and (event != 'REGISTER' and event != 'CONNECTED USERS'):
            #   window['_CLIENT_'].print("c> No text inserted")
            #   continue

            if (client._alias == None or client._username == None or client._alias == 'Text' or client._username == 'Text' or client._date == None) and (event != 'REGISTER'):
                sg.Popup('NOT REGISTERED', title='ERROR', button_type=5, auto_close=True, auto_close_duration=1)
                continue

            if (event == 'REGISTER'):
                client.window_register()

                if (client._alias == None or client._username == None or client._alias == 'Text' or client._username == 'Text' or client._date == None):
                    sg.Popup('NOT REGISTERED', title='ERROR', button_type=5, auto_close=True, auto_close_duration=1)
                    continue

                window['_CLIENT_'].print('c> REGISTER ' + client._alias)
                client.register(client._alias, window)

            elif (event == 'UNREGISTER'):
                window['_CLIENT_'].print('c> UNREGISTER ' + client._alias)
                client.unregister(client._alias, window)


            elif (event == 'CONNECT'):
                window['_CLIENT_'].print('c> CONNECT ' + client._alias)
                client.connect(client._alias, window)


            elif (event == 'DISCONNECT'):
                window['_CLIENT_'].print('c> DISCONNECT ' + client._alias)
                client.disconnect(client._alias, window)


            elif (event == 'SEND'):
                window['_CLIENT_'].print('c> SEND ' + str(Transformar_mensaje(values['_INDEST_'] + " " + values['_IN_'])))

                if (values['_INDEST_'] != '' and values['_IN_'] != '' and values['_INDEST_'] != 'User' and values['_IN_'] != 'Text') :
                    client.send(values['_INDEST_'], values['_IN_'], window)
                else :
                    window['_CLIENT_'].print("Syntax error. Insert <destUser> <message>")


            elif (event == 'SENDATTACH'):

                window['_CLIENT_'].print('c> SENDATTACH ' + values['_INDEST_'] + " " + values['_IN_'] + " " + values['_FILE_'])

                if (values['_INDEST_'] != '' and values['_IN_'] != '' and values['_FILE_'] != '') :
                    client.sendAttach(values['_INDEST_'], values['_IN_'], values['_FILE_'], window)
                else :
                    window['_CLIENT_'].print("Syntax error. Insert <destUser> <message> <attachedFile>")


            elif (event == 'CONNECTED USERS'):
                window['_CLIENT_'].print("c> CONNECTEDUSERS")
                client.connectedUsers(window)



            window.Refresh()

        window.Close()

def enviar_mensaje(window, lista_mensajes,HOST, PORT, isSend=False, isConnectedUsers=False):
    """Función diseñada para enviar mensajes despreocupando el funcionamiento en las propias funciones de la clase
    Con los parámetros se especifica si es un envio normal o es Send o ConnectedUser que necesitan un trato especial"""
    HOST = client._server  # La dirección IP del servidor
    PORT = client._port        # El puerto que utiliza el servidor
    # Crear un socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Conectar al servidor
        try:
            s.connect((HOST, PORT))
            print("Conexión exitosa")
        except socket.error as err:
            return client.RC.ERROR
        n = 0
        contador = 0
        #Enviamos la lista de mensajes
        for mensaje in lista_mensajes:
            contador +=1
            #Si es un send y es el mensaje lo mandamos al servidor web para quitar los espacios
            if contador == 4 and isSend:
                mensaje = Transformar_mensaje(mensaje)
            # Rellena el mensaje con '\0' hasta alcanzar una longitud de 256 caracteres
            mensaje = mensaje.ljust(256, '\0')
            mensaje = mensaje.encode("iso-8859-1")
            s.sendall(mensaje)
        if not isSend:
            #Se captura el error de un byte
            data = s.recv(1)
            n = int(data.decode("iso-8859-1"))
        if isSend:
            #Se captura el error de un byte
            data = s.recv(1)
            data2 = s.recv(11)
            #Capturamos error e id del mensaje
            n = int(data.decode("iso-8859-1").strip('\x00'))
            id = str(data2.decode("iso-8859-1").strip('\x00'))
            return n, id
        #Si es un connectussers tenemos tres casos:

        #Caso de error 1, simplemente se muestra este error
        if isConnectedUsers and n == 1:
            window['_SERVER_'].print("s> CONNECTED USERS FAIL / USER IS NOT CONNECTED")

        #Caso de error 0, fue correcto y actuamos como corresponde
        elif isConnectedUsers and n == 0:
            # Si es conneceted y ha ido bien se recibe lo esperado
            #Se recibe el numero de conectados
            num_con = int(s.recv(12).decode("iso-8859-1", errors='ignore'))
            #Recibimos uno a uno los alias
            string = ""
            for i in range(num_con):
                #Hacemos un rcv por cliente
                cliente = s.recv(256).decode("iso-8859-1").strip('\x00')
                if i == 0:
                    string = string + str(cliente)
                else:
                    string = string + ", " + str(cliente)
            print(string)
            cadena = "s> CONNECTED USERS ( "
            cadena2 = str(num_con) + " ) OK -"
            window['_SERVER_'].print(cadena + cadena2+string)

            window.Refresh()
        #Última posibilidad de error
        elif isConnectedUsers and n==2:
            window['_SERVER_'].print("s> CONNECTED USERS FAIL")
    #Devolvemos el error
    return int(n)

def escuchar_servidor(window, sock):
    """Función que se correrá en un hilo distinto al maestro y permite recoger los mensajes que se envian y los ack"""
    # Bucle de escucha, hacemos este settimeout para controlar si se debe destruir el hilo
    sock.settimeout(0.5)
    #Se escucha
    sock.listen()
    while True:
        #Tratamos de escuchar, si no hay éxito se acaba con el hilo y se cierra el socket
        try:
            #Aceptamos conexiones
            conexion, addr = sock.accept()
            # Recibir del servidor
            operacion = conexion.recv(14).decode("iso-8859-1").strip('\x00')
            #Si es el ack
            if operacion == "SEND_MESS_ACK":
                #Obtemeos el id que viene despues y lo imprimimos por pantalla como se indica
                id = conexion.recv(11).decode("iso-8859-1").strip('\x00')
                #Si entra aquí es porque se estaba tratando de escuchar un ACK
                window['_SERVER_'].print("s> SEND MESSAGE " + str(int(id)) + " OK")
            else:
                #SI no es ack es mensaje asi que recogemos los campos esperados
                alias = conexion.recv(256).decode("iso-8859-1").strip('\x00')

                contador = conexion.recv(11).decode("iso-8859-1").strip('\x00')

                msg = conexion.recv(256).decode("iso-8859-1").strip('\x00')

                cadena = ("s> MESSAGE " + contador + " FROM " + alias)
                cadena2 = "     " + msg
                cadena3 = "     END"
                window['_SERVER_'].print(cadena)
                window['_SERVER_'].print(cadena2)
                window['_SERVER_'].print(cadena3)
                window.Refresh()
        except TimeoutError:
            if cortar_hilo:
                sock.close()
                return

    # Cerramos la conexión
    sock.close()


def Transformar_mensaje(mensaje):
    """Función que se encarga de envíar al servidor web la peticion para quitar los espacios de un mensaje"""
    try:
        #Se conecta al servidor web
        ip_var_entorno = os.getenv('ip_servidor_web')
        wsdl_url = "http://"+str(ip_var_entorno)+":8000/?wsdl"
        soap = Client(wsdl_url)
        # Se ejecuta la función correspondiente del servidor web
        mens_transformado = soap.service.transformar_espacios(mensaje)
        return mens_transformado
    except:
        #Si no hay éxito, se devuelve el mensaje que se pasó
        return mensaje


if __name__ == '__main__':
    client.main([])
    print("+++ FINISHED +++")