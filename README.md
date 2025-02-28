# TCP-UDP-Server-with-Client-Subscriptions-and-Message-Handling
A TCP/UDP server that manages client subscriptions, processes messages from UDP clients, and forwards them to subscribed TCP clients. Implements message parsing, connection handling, and basic command processing.


Andrei Anca-Teodora 323CC

Server.c 

Structuri:

Structura Client: folosita pentru clientii TCP cu urmatoarele campuri: descriptor de fisier asociat socketului, adresa pentru IP si port, sir de caractere pentru id- ul clientului.
Structura Client_udp: folosita pentru clientii UDP cu urmatoarele campuri: descriptor de fisier asociat socketului, adresa pentru IP si port.
Structura Udp_packet: folosita pentru a separa mesajul primit de la Udp in topic, tip si valoare pentru a lucra cu ele conform tabelului din enunt.
Structura topic_struct: folosita pentru a gestiona abonarile la topicuri cu urmatoarele campuri: numele topicului si id-urile clientilor tcp abonati

Functii:

int_to_string: folosita pentru a folosi un int drept un string(dupa ce primesc un int de la udp il strimit ca string la tcp). 
Ea extrage fiecare cifra din numar si o pune in sirul de caractere apoi il inverseaza pt a avea ordinea corecta. Adaug si terminatorul de sir.

mypow: functia putere folosita in locul lui pow. 
Inmultesc baza de atatea ori cat e valoarea exponentului intr un for.

short_real_to_string: transform un numar uint16_t in string pentru al putea trimite in mesajul pentru clientul tcp. 
Aici folosesc sprintf.

start_tcp: creez un socket tcp folosind socket. 
Pun adresa IP a serverului si portul.
Fac bind pentru a lega socketul la server.
Folosesc listen pentru a primi conexiuni de la clienti.
Returnez socketul serverului.

start_udp: la fel ca start_tcp, dar pentru udp

messages: initializez clientii, topicurile si evenimentele.
Astept evenimente de la TCP, UDP sau tastatura folosind poll.
Apoi impart functia in trei pentru gestionarea evenimentelor tcp, udp si stdin.
Pentru socket-urile TCP astept mesaje de subscribe sau unsubscribe. 
Pentru subscribe extrag ce se gaseste dupa cuvantul subscribe ca topic. 
Cu un for care trece prin topice verific daca exista deja acest topic. 
Daca exista spatiu disponibil informatiile socketului sunt copiate in lista de abonati pt acest topic.
Pentru unsubscribe extrag numele topicului.
Caut acest topic in lista, caut si clientul abonat la acest topic si folosesc close.
Pentru socket-urile udp:
Primește și procesează mesaje UDP: impart mesajul primit de la udp in topic, tip si valoare,
conform specificatiilor din enunt legate de cum primesc acest mesaj udp.
Formez mesajul pe care vreau sa il trimit catre clintii tcp potriviti in functie de abonarile lor.
Pentru socketurile stdin verific daca primesc comanda exit, inchid cu close socketurile pentru toti clientii tcp,
apoi inchid si serverul si dau exit.

main: Folosesc setvbuf pentru a dezactiva buffering-ul pentru iesirea standard.
Verific daca numarul de argumente din linia de comanda este corect. 
Se converteste argumentul din linia de comanda, care reprezinta portul, la un intreg folosind atoi.
Initializez doua socket-uri pentru TCP si UDP folosind start_tcp si start_udp.
Initializez vectorul de clienti.
Apelez functia principala de gestionare, adica messages.
Inchid socketurile.

subscribe.c
Funcția start_client face conexiunea la server, dezactiveaza algoritmului Nagle.
In funcția run_client realizeaz comunicarea intre client si server. Folosesc funcția poll pentru evenimente de la socketul de citire si de la tastatura. 
Folosesc doua pachete send_packet si receive_packet pentru a trimite mesaje de la tcp la server si pt a primi raspunsuri de la server.
In main verific argumentele, initializez clientul, aoelez run_client care e practic functia care gestioneaza comunicarea si la final inchid conexiunea.





