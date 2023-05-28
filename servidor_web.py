from spyne import Application, ServiceBase, Unicode, rpc
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication
from wsgiref.simple_server import make_server

class Quitar_espacios(ServiceBase):
    # Se definen las funciones del servicio web
    @rpc(Unicode, _returns=Unicode)
    def transformar_espacios(ctx, mensaje):
        # Se transforma el mensaje
        new_text = ' '.join(mensaje.split())
        return new_text

# Se definen las acciones a realizar
application = Application(
    services=[Quitar_espacios],
    tns='http://tests.python-zeep.org/',
    in_protocol=Soap11(validator='lxml'),
    out_protocol=Soap11()
)

# Se crea WsgiApplication a partir de la definida anteriormente
application = WsgiApplication(application)

if __name__ == '__main__':
    # Se inicializa el servidor con los parÃ¡metros correspondientes
    servidor_web = make_server('0.0.0.0', 8000, application)
    # Se inicia el servidor para escucahr a los clientes
    servidor_web.serve_forever()