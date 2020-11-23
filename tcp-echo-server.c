/*
Titel: tcp_echo_server
Beschreibung: Die Funktion tcp_echo_server erstellt einen Server, welcher Daten mit einer
Maximallaenge von 100 Bytes von Clients entgegennimmt und diese rle-codiert zurückschickt.
Der Empfang von 'Enter' beendet den Serverdienst.
Autor: Patrick Wintner
Datum der letzten Bearbeitung: 22.11.2020
*/

// -- Standardbibliotheken --
#include <stdlib.h>
#include <stdio.h>

// -- Netzwerkbibliotheken --
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// -- andere Bibliotheken --
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define PORT 12345

// -- Funktion *encode --
// Parameter: char *str ... zu komprimierender String
// Beschreibung: siehe oben
// Rueckgabewert: Adresse des komprimierten Strings
char *encode(char *str)
{
	char *result_strt = malloc(2*sizeof(str)); // Allozieren des maximal benoetigten Speicherplatzes
	char *result; 
	result = result_strt; 
	
	char *str_temp; // zum temporaeren Speichern der Adresse und des letzten gleichen Zeichens
	str_temp = str++; // Speichern der Startadresse auf str_temp

	for(; *str != '\0'; str++)
	{
		if(*str_temp != *str)
		{
			*result = *str_temp;
			sprintf(++result, "%i", (str-str_temp));
			if(str-str_temp == 100)
			{
				sprintf(++result, "0");
			}
			if(str-str_temp >= 10)
			{
				sprintf(++result, "%i", (str-str_temp)%10);
			}
			result++;
			str_temp = str;
		}	
	}
	*result = *str_temp;
	sprintf(++result, "%i", (str-str_temp));

	return result_strt;
}

// -- Funktion delay --
// Parameter:
//	* int milisec ... Dauer der Verzoegerung in Milisekunden
// Beschreibung:
//	delay verzoegert die Ausfuehrung des Programmes um die durch den Parameter milisec
//	angegebene Dauer in Milisekunden.
// Rueckgabewert: -
void delay(int milisec)
{
	clock_t start_time = clock();
	while(clock() < start_time + milisec);
}

// -- Funktion clr_str --
// Parameter: *string ... zu saeubernder String
// Beschreibung: clrstr ersetzt \n mit \0 (\n entsteht durch Enter bei Eingabe)
// Rueckgabewert: -
void clr_str(char *string)
{
	for(int i = 0; ;i++)
	{
		if(string[i] == '\n') 
		{
			string[i] = '\0';
			string[i+1] = '\0'; // Aus einem mir unbekannten Grund haengt
			// "sprintf(buf_decode, "%s", encode(buf));" immer nach \n eine '1' an, 
			//welche hier entfernt wird
			break;
		}
	}
}

// -- Funktion tcp_echo_server --
// Parameter:
//	* int port ... gibt die Portnummer an, an welcher der Server Anfragen entgegennimmt;
//	Minimalwert: 1025 (aufgrund well-known Ports)
//	* bool sys_message_en ... enabelt(true)/disabelt(false) Systemmeldungen
//	* bool err_message_en ... enabelt(true)/disabelt(false) Fehlermeldungen
// Beschreibung:
//	tcp_echo_server erstellt ein Server, welcher an die durch den Parameter port
//	bestimmten Port gebunden ist und Anfragen mit beliebiger IP-Adresse entgegennimmt.
//	Die Maximalanzahl an anstehenden Client-Anfragen betraegt 5. Die Maximallaenge einer
//	Nachricht betraegt 100 Bytes - sollte die empfangenen Daten laenger sein, so wird dem
//	Sender eine Fehlermeldung zurueckgeschickt. Der Server antwortet auf Nachrichten,
//	indem er den Inhalt einer empfangenen Nachricht rle-codiert und dann an den Sender
//	zurueckschickt. Auftretende Fehler werden durch Fehlermeldungen angezeigt, sofern diese
//	aktiviert sind, als auch ueber den Rueckgabewert kodiert, wie weiter unten erlaeutert.
//	Sollte eine empfangene Nachricht als erstes Zeichen ein '\n' beinhalten (Also wenn der
//	Sender, sollte er die Eingabe ueber die Tastatur durchgefuehrt haben, lediglich 'Enter' gedrueckt
//	haben), so wird der Serverdienst beendet.
// Rueckgabewert:
//	... wird zur Fehlercodierung verwendet:
//	* 0: Beendigung des Serverdienstes durch das Empfangen von 'Enter'
//	* -1: ungueltige Port-Nummer
//	* -2: Fehler beim Anlegen des Server-Sockets
//	* -3: Fehler beim Binden des Sockets
//	* -4: Fehler bei listen
//	* -5: Fehler beim Akzeptieren einer Client-Anfrage
//	* -6: Fehler beim Empfangen
//	* -7: Fehler beim Senden
int tcp_echo_server(int port, bool sys_message_en, bool err_message_en)
{
	if(port <= 1024) // aufgrund moeglicher Konflikte mit well-known Ports
	{
		if(err_message_en == true) printf("ungueltige Port-Nummer");
		return -1;
	}

	int fd_server, fd_client; // File-Deskriptoren fuer Server und Client
	struct sockaddr_in addr_server, addr_client; //Anlegen der structs fuer Server und Client
	char buf[102]; // 100+3-Byte-Buffer; erlaubt Empfang und Senden von 100 Bytes+\n+\0+?
	char buf_decode[102]; // 100+3-Byte-Buffer; erlaubt Senden von 100 Bytes+\n+\0+?
	int len; // Groesse von addr_client
	int n_Byte; // Anzahl empfangener Bytes

	addr_server.sin_family = AF_INET; // legt Adressenfamilie fest, in diesem Fall IPv4
	addr_server.sin_port = htons(port); // definiert den Port des Server-Sockets
	addr_server.sin_addr.s_addr = htonl(INADDR_ANY); // Server akzeptiert Anfragen mit
	// beliebiger IP-Adresse

	fd_server = socket(AF_INET, SOCK_STREAM, 0); // Anlegen des Server-Sockets
	// AF_INET ... Adressenfamilie (IPv4)
	// SOCK_STREAM ... Verwendung von TCP
	// 0 ... Verwendung des (Standard-) Protokolls PF_INET
	if(fd_server == -1) // Fehlerpruefung
	{
		if(err_message_en == true) printf("Fehler beim Anlegen des Server-Sockets");
		return -2;
	}
	if(sys_message_en == true) printf("Server-Socket angelegt\n");
	if(bind(fd_server, (struct sockaddr *) &addr_server, sizeof(addr_server)) == -1)
	// Dem Server-Socket wird ein "Name" (Adresse und Port) zugewiesen
	{
		if(err_message_en == true) printf("Fehler beim Binden des Sockets");
		close(fd_server);
		return -3;
	}
	if(sys_message_en == true) printf("Server-Socket an Adresse und Port gebunden\n");
	if(listen(fd_server, 5) == -1) // Der Server wartet an dem ihm zugewiesenem Port auf
	// Client-Anfragen
	{
		if(err_message_en == true) printf("Fehler bei listen");
		close(fd_server);
		return -4;
	}
	if(sys_message_en == true) printf("Warten auf Anfragen...\n");
	for(;;)
	{
		len = sizeof(addr_client);
		fd_client = accept(fd_server, (struct sockaddr *) &addr_client, &len);
		// Der Server akzeptiert die erste anstehende Client-Anfrage und erstellt ein
		// neues Socket fd_client mit den selben Eigenschaften wie fd_server; accept
		// blockt das Programm, bis eine Anfrage eintrifft.
		memset(buf, 0, sizeof(buf));
		memset(buf_decode, 0, sizeof(buf_decode));
		// Der Inhalt der Buffers wird resettet
		if(fd_client == -1)
		{
			if(err_message_en == true) printf("Fehler beim Akzeptieren einer Client-Anfrage");
			close(fd_client);
			close(fd_server);
			return -5;
		}
		if(sys_message_en == true) printf("Anfrage von %s erhalten\n", inet_ntoa(addr_client.sin_addr));
		// inet_ntoa: wandelt die 32-Bit-Darstellung der IP-Adresse zur dotted-decimal-Darstellung (String) um
		n_Byte = recv(fd_client, buf, sizeof(buf), 0);
		if(n_Byte <= 101) // Der Server wartet auf eine
		// Nachricht vom Client, welcher er dann in den Buffer buf schreibt. Sollte
		// diese groesser als buf sein, werden ueberzaehlige Bytes verworfen.
		// Warum 101? Wegen "\n"
		{
			if(n_Byte == -1)
			{
				if(err_message_en == true) printf("Fehler beim Empfangen");
				close(fd_client);
				close(fd_server);
				return -6;
			}
			if(buf[0] == '\n') // Beendigung der Serverdienstes
			{
				close(fd_client);
				break;
			}
			if(sys_message_en == true) printf("Erhaltene Nachricht: %s\n", buf);
			sprintf(buf_decode, "%s", encode(buf)); // Dekodieren des Nachrichteninhalts
			clr_str(buf_decode); // Ersetzen von \n1 durch \0
			if(sys_message_en == true) printf("Dekodierte Nachricht: %s\n", buf_decode);
			if(send(fd_client, buf_decode, sizeof(buf_decode), 0) == -1) // Senden des Inhalts
			//des dekodierten Buffers buf an den Client
			{
				if(err_message_en == true) printf("Fehler beim Senden");
				close(fd_client);
				close(fd_server);
				return -7;
			}
			else 
			{
				delay(100); // unter Umstaenden wuerde die Verbindung mit
				// dem Client geschlossen werden, ehe die Nachricht gesendet
				// wuerde - dies wird durch die Verzoegerung verhindert
				close(fd_client); // Schliessen der Verbindung mit dem Client
			}
		}
		else
		{
			if(sys_message_en == true) printf("Erhaltene Nachricht >= 100 Bytes\n");
			memset(buf, 0, sizeof(buf)); // Ruecksetzen des Inhalts des Buffers
			sprintf(buf, "Fehler: maximale Nachrichtengroesse von 100 Bytes ueberschritten");
			if(send(fd_client, buf, sizeof(buf), 0) == -1) // Senden der Fehlermeldung
			// des Buffers an den Client
			{
				if(err_message_en == true) printf("Fehler beim Senden");
				close(fd_client);
				close(fd_server);
				return -7;
			}
			else 
			{
				delay(100); // unter Umstaenden wuerde die Verbindung mit
				// dem Client geschlossen werden, ehe die Nachricht versendet
				// wurde - dies wird durch die Verzoegerung verhindert
				close(fd_client); // Schliessen der Verbindung mit dem Client
			}
		}
	}
	close(fd_server); // Schliessen des Serversockets
	if(sys_message_en == true) printf("Serverdienst wird beendet...");
	return 0;
}
int main()
{
	tcp_echo_server(PORT, true, true); // Starten des Servers mit enabelten Fehler- und
	// System-Meldungen und der Portnummer 12345
	return 0;
}