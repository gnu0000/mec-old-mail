/*
 *
 * mail.c
 * Monday, 4/21/1997.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock.h>
#include <GnuType.h>


#define FAILED_TO_CONNECT              1
#define FAILED_TO_SEND                 2
#define FAILED_TO_RECEIVE              3
#define SERVER_ERROR                   4
#define FAILED_TO_START_SOCKETS        5
#define FAILED_TO_OBTAIN_SOCKET_HANDLE 6
#define FAILED_TO_RESOLVE_HOST         7
#define FAILED_TO_GET_HOSTNAME         8

CHAR szPUTBUFF [4096];


INT Post (SOCKET s, PSZ pszMsg)
   {
   INT iLen, iBlock;

   for (iLen = strlen (pszMsg); iLen; iLen -= iBlock)
      {
      if ((iBlock = send (s, pszMsg, iLen, 0)) < 1)
         return FAILED_TO_SEND;
      pszMsg += iBlock;
      }
   return 0;
   }

INT Ack (SOCKET s)
   {
   INT  iLen, iBlock;
   CHAR szBuff [4096];
   PSZ  pszMsg;

   printf ("in ack \n");
   for (iLen = 0, pszMsg = szBuff; ; iLen -= iBlock)
      {
      if ((iBlock = recv (s, pszMsg, 4096 - iLen - 1, 0)) < 1)
         return FAILED_TO_RECEIVE;
      pszMsg += iBlock;

      if ((pszMsg[-2] == '\r') && (pszMsg[-1] == '\n'))
         break;
      }
   if (*szBuff > '3')
      return SERVER_ERROR;
   return 0;
   }


INT Put (SOCKET s, PSZ psz, ...)
   {
   va_list vlst;
   INT     iRet;

   va_start (vlst, psz);
   vsprintf (szPUTBUFF, psz, vlst);
   va_end (vlst);

   if (iRet = Post (s, szPUTBUFF))
      return iRet;
   return Ack (s);
   }


unsigned long GetAddr (PSZ szHost) 
   {
   LPHOSTENT lpstHost;
   u_long    lAddr = INADDR_ANY;

   printf ("in getaddr \n");
   if (!*szHost) 
      return INADDR_ANY;
   
   /* check for a dotted-IP address string */
   lAddr = inet_addr (szHost);

   /* If not an address, then try to resolve it as a hostname */
   if ((lAddr != INADDR_NONE) || (!strcmp(szHost, "255.255.255.255"))) 
      return lAddr; 

   if (lpstHost = gethostbyname(szHost)) 
      return  *((u_long FAR *) (lpstHost->h_addr));
   else 
      return INADDR_ANY;   /* failure */
   }



INT SendMail (void)
   {
   struct hostent *adr;
   WSADATA     Data;
   SOCKET      s;
   SOCKADDR_IN sin;
   char        szLocalHost [256];

   printf ("in sendmail \n");

   printf ("startup\n");
   if (WSAStartup(MAKEWORD(1, 1), &Data))
      return FAILED_TO_START_SOCKETS;

   printf ("open socket\n");
   if ((s = socket(PF_INET,SOCK_STREAM, 0)) == INVALID_SOCKET)
      return FAILED_TO_OBTAIN_SOCKET_HANDLE;

   // Resolve the servers IP
   printf ("get host by name\n");
   if (!(adr = gethostbyname("mail.cloverleaf.net")))
      return FAILED_TO_RESOLVE_HOST;

   // Connect to server
   printf ("connect\n");
   sin.sin_family           = AF_INET;
   sin.sin_port             = htons(25);
   sin.sin_addr.S_un.S_addr = GetAddr("mail.cloverleaf.net");
   if (connect (s, (LPSOCKADDR)&sin, sizeof(sin)))
      return FAILED_TO_CONNECT;

   Ack(s); // Server welcome message

   printf ("get host name\n");
   if (gethostname(szLocalHost, 255))
      return FAILED_TO_GET_HOSTNAME;

   Put (s, "HELO %s\r\n", szLocalHost); 
   Put (s, "MAIL FROM:<%s>\r\n", "craig"); 
   Put (s, "RCPT TO:<%s>\r\n", "craig.fitzgerald@cloverleaf.net"); 
   Put (s, "DATA\r\n");  

   Post(s, "From: Craig Fitzgerald <Craig@Cloverleaf.net>\r\n");
   Post(s, "To:   Craig Fitzgerald <Craig@Cloverleaf.net>\r\n");
   Post(s, "Date: Mon, 21 Apr 1997 02:03:04 -0500\r\n");
   Post(s, "Subject: Testing\r\n");
   Post(s, "\r\n");
   Post(s, "This is a test message body");
   Post(s, "\r\n");
   Post(s, ".\r\n");
   Ack(s);

   return 0;
   }

int main (int argc, char *argv[])
   {
   int iRet;

   iRet = SendMail ();
   printf ("iRet = %d\n", iRet);
   
   return 0;
   }
