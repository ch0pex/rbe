Vale frame tail:

- Tercer parametro de rbe::frames::frame
- sirve para desacoplar el final de los protocolos del body
- util para:
  - protocolos con crc
  - sbe con su block length

El frame debe aceptar un tercer parametro opcional que sera la cola o footer
del frame. Este se mantendra siempre al final independientemente del payload.
Dado que hay frames que son buffer delimited y otros que no son O(1) estos no podran
tener tail. El resto de frames podra tener tail y encima el header puede especificar
justamente donde termina el payload:

- frame_length pasara a ser:    frame_length    = header_length + payload_length + tail_length
- header_length pasara a ser:   header_length   = frame_length - payload_length - tail_length
- payload_length pasara a ser:  payload_length  = frame_length - header_length - tail_length
- tail_length pasara a ser:     tail_length     = frame_length - header_length - payload_length
Si el frame no obtiene tail, tail_length sera 0 y el resto de formulas se mantendran igual.
De momento un tail podra ser un wirable, un buffer (para type erasure), ... ?
