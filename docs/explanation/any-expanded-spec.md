# RBE Feature proposals: Any supports all payload types

### Motivation

Any ahora mismo solo soporta wirables, sin embargo esto limita bastante el soporte a diferentes protocolos. Permitir  
los mismos tipos que payload mejora la expresividad de la libreria y permite que se pueda usar en mas protocolos.

Por ejemplo, hay protocolos donde quiza hay mensajes que tienen que ser manejados de forma especial mediante una clase
custom o simplemente como un buffer de bytes. A dia de hoy para estos casos imposibilita el uso de la biblioteca, si
se ofrece soporte para mas tipos en any se puede dar soporte a dichos caso como por ejemplo:

struct msg_1 { ... }; // wirable
struct msg_2 { ... }; // wirable
struct msg_3 { ... }; // wirable

class msg_4 {  
public:
 explicit constexpr msg_4(std::span<std::byte const> const data) {}

private:
  ...
}; // custom class

using any = rbe::frames::any<msg_1, msg_2, msg_3, msg_4>;

Esto facilitara la adopcion de la libreria en protocolos donde todavia no se tenga un soporte completo
para cubrir determinados casos de uso. Como el caso de opra donde hay mensajes que no se pueden manejar
con wirables porque tienen fields opcionales.
