
// Send an IPv4 ICMP echo request packet via raw socket at the link layer (ethernet frame),
// and receive echo reply packet (i.e., ping). Includes some ICMP data.
// Need to have destination MAC address. ?

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_ICMP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/ip_icmp.h>  // struct icmp, ICMP_ECHO
#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>
#include <sys/time.h>         // gettimeofday()

#include <errno.h>            // errno, perror()

// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP4_HDRLEN 20  // IPv4 header length
#define ICMP_HDRLEN 8  // ICMP header length for echo request, excludes data

#define KYEL "\x1B[0;33m"    // Yellow
#define KRED "\x1B[0;31m"    // Red > Alive
#define KBLU "\x1B[0;34m"    // Blue > Unreachable 
#define KGRN "\x1B[0;32m"    // Me : My IP  


// Function prototypes
uint16_t checksum (uint16_t *, int);
uint16_t icmp4_checksum (struct icmp, uint8_t *, int);
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);
int *allocate_intmem (int);

int main (int argc, char **argv)
{
  int i, status, datalen, frame_length, sendsd, recvsd, bytes, *ip_flags, timeout, trycount, trylim, done;
  char *interface, *target, *src_ip, *dst_ip, *rec_ip;
  struct ip send_iphdr, *recv_iphdr;
  struct icmp send_icmphdr, *recv_icmphdr;
  uint8_t *data, *src_mac, *dst_mac, *send_ether_frame, *recv_ether_frame;
  struct addrinfo hints, *res;
  struct sockaddr_in *ipv4;
  struct sockaddr_ll device;
  struct ifreq ifr;
  struct sockaddr from;
  socklen_t fromlen;
  struct timeval wait, t1, t2;
  struct timezone tz;
  double dt;
  void *tmp;

  // Allocate memory for various arrays.
  src_mac = allocate_ustrmem (6);
  dst_mac = allocate_ustrmem (6);
  data = allocate_ustrmem (IP_MAXPACKET);
  send_ether_frame = allocate_ustrmem (IP_MAXPACKET);
  recv_ether_frame = allocate_ustrmem (IP_MAXPACKET);
  interface = allocate_strmem (40);
  target = allocate_strmem (40);
  src_ip = allocate_strmem (INET_ADDRSTRLEN);
  dst_ip = allocate_strmem (INET_ADDRSTRLEN);
  rec_ip = allocate_strmem (INET_ADDRSTRLEN);
  ip_flags = allocate_intmem (4);
  
  if(argc==5) // ./ipscanner -i interface -t timeout 
  {	  
  	// Interface to send packet through.
  	strcpy (interface, argv[2]);

	// Submit request for a socket descriptor to look up interface.
	// We'll use it to send packets as well, so we leave it open.
	if ((sendsd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) 
    {
	    perror ("socket() failed to get socket descriptor for using ioctl() ");
	    exit (EXIT_FAILURE);
	}

	// Use ioctl() to look up interface name and get its MAC address.
	memset (&ifr, 0, sizeof (ifr));
	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
	if (ioctl (sendsd, SIOCGIFHWADDR, &ifr) < 0) 
    {
	    perror ("ioctl() failed to get source MAC address ");
	    return (EXIT_FAILURE);
	}

	// Copy source MAC address.
	memcpy (src_mac, ifr.ifr_hwaddr.sa_data, 6);

	// Report source MAC address to stdout.
	/*
	  printf ("MAC address for interface %s is ", interface);
	  for (i=0; i<5; i++) 
      {
	    printf ("%02x:", src_mac[i]);
	  }
	  printf ("%02x\n", src_mac[5]);
	*/

	 // Find interface index from interface name and store index in
	 // struct sockaddr_ll device, which will be used as an argument of sendto().
     
	 memset (&device, 0, sizeof (device));
	 if ((device.sll_ifindex = if_nametoindex (interface)) == 0)
         {
	   perror ("if_nametoindex() failed to obtain interface index ");
	   exit (EXIT_FAILURE);
	 }
     
	 // printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);

	 // Set destination MAC address
	 dst_mac[0] = 0xff;
	 dst_mac[1] = 0xff;
	 dst_mac[2] = 0xff;
	 dst_mac[3] = 0xff;
	 dst_mac[4] = 0xff;
	 dst_mac[5] = 0xff;

	 // Source IPv4 address
	 strcpy (src_ip, "140.117.172.88");

         int alive_cnt = 0;
	 

	 // Destination "URL" or "IPv4 address"
	 // run subnet ip range from .1~.254 except myself
	 for(int i=1;i<=254;i++)
	 {
          
	     if(i==88)
	     {
		     printf(KGRN"\nIS ME: 140.117.172.88\n\n");
		     continue; // 下一位 
 	     }	     
         
	     strcpy(target,"140.117.172."); // 先寫好 prefix 
	     char tail[3];
         sprintf(tail,"%d",i); // 尾碼 , 數字轉字串 	     
         strcat(target,tail);  // 字串相接: prefix + 尾碼
         //printf("target: %s\n",target); // 送到哪兒
	     
	     // Fill out hints for getaddrinfo().
	     memset (&hints, 0, sizeof (struct addrinfo));
	     hints.ai_family = AF_INET;
	     hints.ai_socktype = SOCK_STREAM;
	     hints.ai_flags = hints.ai_flags | AI_CANONNAME;

	     // Resolve target using getaddrinfo().
	     if ((status = getaddrinfo (target, NULL, &hints, &res)) != 0) 
         {
	       fprintf (stderr, "getaddrinfo() failed: %s\n", gai_strerror (status));
	       exit (EXIT_FAILURE);
	     }
	     ipv4 = (struct sockaddr_in *) res->ai_addr;
	     tmp = &(ipv4->sin_addr);
	     if (inet_ntop (AF_INET, tmp, dst_ip, INET_ADDRSTRLEN) == NULL)
         {
	       status = errno;
	       fprintf (stderr, "inet_ntop() failed.\nError message: %s", strerror (status));
	       exit (EXIT_FAILURE);
	     }
	     freeaddrinfo (res);

	     // Fill out sockaddr_ll.
	     device.sll_family = AF_PACKET;
	     memcpy (device.sll_addr, src_mac, 6);
	     device.sll_halen = 6;

	     // ICMP data // 學號 
	     datalen = 11;
             //data = "M083040017"; // 會有 free invalid pointer question
	     data[0] = 'M';
	     data[1] = '0';
	     data[2] = '8';
	     data[3] = '3';
	     data[4] = '0';
	     data[5] = '4';
	     data[6] = '0';
	     data[7] = '0';
	     data[8] = '1';
	     data[9] = '7';

	     // IPv4 header //
	     // IPv4 header length (4 bits): Number of 32-bit words in header = 5
	     send_iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t);
	     // Internet Protocol version (4 bits): IPv4
	     send_iphdr.ip_v = 4;

	     // Type of service (8 bits)
	     send_iphdr.ip_tos = 0;

	     // Total length of datagram (16 bits): IP header + ICMP header + ICMP data
	     send_iphdr.ip_len = htons (IP4_HDRLEN + ICMP_HDRLEN + datalen);

	     // ID sequence number (16 bits): unused, since single datagram 
	     send_iphdr.ip_id = htons (0);

	     // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

	     // Zero (1 bit)
	     ip_flags[0] = 0;

	     // Do not fragment flag (1 bit)
	     ip_flags[1] = 0;

	     // More fragments following flag (1 bit)
	     ip_flags[2] = 0;

	     // Fragmentation offset (13 bits)
	     ip_flags[3] = 0;

	     send_iphdr.ip_off = htons ((ip_flags[0] << 15)
	                      + (ip_flags[1] << 14)
	                      + (ip_flags[2] << 13)
	                      +  ip_flags[3]);

	     // Time-to-Live (8 bits): default to maximum value > to be continue
	     send_iphdr.ip_ttl = 255;

	     // Transport layer protocol (8 bits): 1 for ICMP
	     send_iphdr.ip_p = IPPROTO_ICMP;

	     // Source IPv4 address (32 bits)
	     if ((status = inet_pton (AF_INET, src_ip, &(send_iphdr.ip_src))) != 1)
         {
            fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
            exit (EXIT_FAILURE);
	     }

	     // Destination IPv4 address (32 bits)
	     if ((status = inet_pton (AF_INET, dst_ip, &(send_iphdr.ip_dst))) != 1) {
	       fprintf (stderr, "inet_pton() failed.\nError message: %s", strerror (status));
	       exit (EXIT_FAILURE);
	     }

	     // IPv4 header checksum (16 bits): set to 0 when calculating checksum
	     send_iphdr.ip_sum = 0;
	     send_iphdr.ip_sum = checksum ((uint16_t *) &send_iphdr, IP4_HDRLEN);

         // ICMP header //

	     // Message Type (8 bits): echo request
	     send_icmphdr.icmp_type = ICMP_ECHO;

	     // Message Code (8 bits): echo request
	     send_icmphdr.icmp_code = 0;

	     // Identifier (16 bits): usually pid of sending process - pick a number
	     send_icmphdr.icmp_id = htons (1000); // 0x2657

	     // Sequence Number (16 bits): starting from 1 
	     send_icmphdr.icmp_seq = htons (1);

	     // ICMP header checksum (16 bits): set to 0 when calculating checksum
	     send_icmphdr.icmp_cksum = icmp4_checksum (send_icmphdr, data, datalen);

	     // Fill out ethernet frame header.

	     // Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (IP header + ICMP header + ICMP data)
	     frame_length = 6 + 6 + 2 + IP4_HDRLEN + ICMP_HDRLEN + datalen;

	     // Destination and Source MAC addresses
	     memcpy (send_ether_frame, dst_mac, 6);
	     memcpy (send_ether_frame + 6, src_mac, 6);

	     // Next is ethernet type code (ETH_P_IP for IPv4).  
	     send_ether_frame[12] = ETH_P_IP / 256;
	     send_ether_frame[13] = ETH_P_IP % 256;

	     // Next is ethernet frame data (IPv4 header + ICMP header + ICMP data).

	     // IPv4 header
	     memcpy (send_ether_frame + ETH_HDRLEN, &send_iphdr, IP4_HDRLEN);

	     // ICMP header
	     memcpy (send_ether_frame + ETH_HDRLEN + IP4_HDRLEN, &send_icmphdr, ICMP_HDRLEN);

	     // ICMP data
	     memcpy (send_ether_frame + ETH_HDRLEN + IP4_HDRLEN + ICMP_HDRLEN, data, datalen);

	     // Submit request for a raw socket descriptor to receive packets.
	     if ((recvsd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0)
         {
	       perror ("socket() failed to obtain a receive socket descriptor ");
	       exit (EXIT_FAILURE);
	     }

	     // Set maximum number of tries to ping remote host before giving up.
	     trylim = 4;
	     trycount = 0;

	     // Cast recv_iphdr as pointer to IPv4 header within received ethernet frame.
	     recv_iphdr = (struct ip *) (recv_ether_frame + ETH_HDRLEN);

	     // Cast recv_icmphdr as pointer to ICMP header within received ethernet frame.
	     recv_icmphdr = (struct icmp *) (recv_ether_frame + ETH_HDRLEN + IP4_HDRLEN);

	     done = 0;
	     for (;;) 
         {

            // SEND

            // Send ethernet frame to socket.
            if ((bytes = sendto (sendsd, send_ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) {
              perror ("sendto() failed ");
              exit (EXIT_FAILURE);
            }

            // Start timer.
            (void) gettimeofday (&t1, &tz);

            // Set time for the socket to timeout and give up waiting for a reply.
            timeout = atoi(argv[4]);
            wait.tv_sec  = timeout;  
            wait.tv_usec = 0;
            setsockopt (recvsd, SOL_SOCKET, SO_RCVTIMEO, (char *) &wait, sizeof (struct timeval));

            // Listen for incoming ethernet frame from socket recvsd.
            // We expect an ICMP ethernet frame of the form:
            //     MAC (6 bytes) + MAC (6 bytes) + ethernet type (2 bytes)
            //     + ethernet data (IPv4 header + ICMP header)
            // Keep at it for 'timeout' seconds, or until we get an ICMP reply.
            
	    struct timeval start;
	    struct timeval end;


            // RECEIVE LOOP
            for (;;) 
            {
                 gettimeofday(&start,NULL); // 開始計時

                  memset (recv_ether_frame, 0, IP_MAXPACKET * sizeof (uint8_t));
                  memset (&from, 0, sizeof (from));
                  fromlen = sizeof (from);
                  if ((bytes = recvfrom (recvsd, recv_ether_frame, IP_MAXPACKET, 0, (struct sockaddr *) &from, &fromlen)) < 0) 
                  {
                        status = errno;
                        // Deal with error conditions first.
                        if (status == EAGAIN) {  // EAGAIN = 11
                          printf ("No reply within %i seconds.\n", timeout);
                          trycount++;
                          break;  // Break out of Receive loop.
                        } else if (status == EINTR) {  // EINTR = 4
                          continue;  // Something weird happened, but let's keep listening.
                        } else {
                          perror ("recvfrom() failed ");
                          exit (EXIT_FAILURE);
                        }
                  }  // End of error handling conditionals.
                  // printf("trycount: %d\n",trycount);
                  // printf("check\n\n");
                  // Check for an IP ethernet frame, carrying ICMP echo reply. If not, ignore and keep listening.
                  
                  if ((((recv_ether_frame[12] << 8) + recv_ether_frame[13]) == ETH_P_IP) &&
                     (recv_iphdr->ip_p == IPPROTO_ICMP) && (recv_icmphdr->icmp_type == ICMP_ECHOREPLY) && (recv_icmphdr->icmp_code == 0)) 
                  {

                        // Stop timer and calculate how long it took to get a reply.
                        (void) gettimeofday (&t2, &tz);
                        dt = (double) (t2.tv_sec - t1.tv_sec) * 1000.0 + (double) (t2.tv_usec - t1.tv_usec) / 1000.0;

                        // Extract source IP address from received ethernet frame.
                        if (inet_ntop (AF_INET, &(recv_iphdr->ip_src.s_addr), rec_ip, INET_ADDRSTRLEN) == NULL) {
                          status = errno;
                          fprintf (stderr, "inet_ntop() failed.\nError message: %s", strerror (status));
                          exit (EXIT_FAILURE);
                        }

                        // Report source IPv4 address and time for reply.
                    //printf("type: %d\n",recv_icmphdr->icmp_type);
                    //printf("code: %d\n",recv_icmphdr->icmp_code);
                        //printf ("%s  %g ms (%i bytes received)\n", rec_ip, dt, bytes);
                        printf(KYEL"PING %s (data size = 10, id = 0x2657 ,seq = %d ,timeout = 10000 ms)\n",target,i);
                        printf(KRED"\tReply from : %s ,time : %g ms\n",target,dt);
			alive_cnt++;
                        done = 1;
                        break;  // Break out of Receive loop.
                  }  // End if IP ethernet frame carrying ICMP_ECHOREPLY
		  /*
                  // We ran out of tries, so let's give up.
                  if (trycount == trylim) 
                  {
                        //printf("type: %d\n",recv_icmphdr->icmp_type);
                        //printf("code: %d\n",recv_icmphdr->icmp_code);
                        if(recv_icmphdr->icmp_type==3)
                        {
                            printf("The host Unreachable\n");
                        }
                        //printf ("Unreachable | Recognized no echo replies from remote host after %i tries.\n", trylim);
                        printf(KYEL"PING %s (data size = 10, id = 0x2657 ,seq = %d,  timeout = 10000 ms)\n",target,i);
                        printf(KBLU"\tDestination unreachable\n");
                        done = 1;  
                        break;
                  }
                  trycount = trycount + 1;
                  //printf("test\n");
		  */
                  gettimeofday(&end,NULL);
		  long int diff;
		  diff = 1000000*(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec);
		  //printf("diff: %ld\n",diff);
		  if(diff>atoi(argv[4]))
		  {
			 // printf("timeout!\n");
			  printf(KYEL"PING %s (data size = 10, id = 0x2657 ,seq = %d,  timeout = 10000 ms)\n",target,i);
                          printf(KBLU"\tDestination unreachable\n");
                          done = 1;
			  break;
		  }
            }  // End of Receive loop.

            // The 'done' flag was set because an echo reply was received; break out of send loop.
            if (done == 1) 
            {
               break;  // Break out of Send loop.
            }



	  }  // End of Send loop.

	
	} // send to all (except src:me) end
      printf("Number of Alive: %d\n",alive_cnt);
      // Close socket descriptors.
	  close (sendsd);
	  close (recvsd);

	  // Free allocated memory.
	  
	  free (src_mac);
	  //printf("test1\n");
	  free (dst_mac);
	  // printf("test2\n");
	  free (data);
	  // printf("test3\n");
	  free (send_ether_frame);
	  // printf("test4\n");
	  free (recv_ether_frame);
	  // printf("test5\n");
	  free (interface);
	   //printf("test6\n");
	  free (target);
	   //printf("test7\n");
	  free (src_ip);
	   //printf("test8\n");
	  free (dst_ip);
	   //printf("test9\n");
	  free (rec_ip);
	   //printf("test9\n");
	  free (ip_flags);
	  //printf("test10\n"); 
	  return (EXIT_SUCCESS);
    } // end argc == 5 : end program	  
	else // 格式不符 
    {
       printf(KRED"Format Error\n");
    }        
	  
} // end main

// Build IPv4 ICMP pseudo-header and call checksum function.
uint16_t
icmp4_checksum (struct icmp icmphdr, uint8_t *payload, int payloadlen)
{
  char buf[IP_MAXPACKET];
  char *ptr;
  int chksumlen = 0;
  int i;

  ptr = &buf[0];  // ptr points to beginning of buffer buf

  // Copy Message Type to buf (8 bits)
  memcpy (ptr, &icmphdr.icmp_type, sizeof (icmphdr.icmp_type));
  ptr += sizeof (icmphdr.icmp_type);
  chksumlen += sizeof (icmphdr.icmp_type);

  // Copy Message Code to buf (8 bits)
  memcpy (ptr, &icmphdr.icmp_code, sizeof (icmphdr.icmp_code));
  ptr += sizeof (icmphdr.icmp_code);
  chksumlen += sizeof (icmphdr.icmp_code);

  // Copy ICMP checksum to buf (16 bits)
  // Zero, since we don't know it yet
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  chksumlen += 2;

  // Copy Identifier to buf (16 bits)
  memcpy (ptr, &icmphdr.icmp_id, sizeof (icmphdr.icmp_id));
  ptr += sizeof (icmphdr.icmp_id);
  chksumlen += sizeof (icmphdr.icmp_id);

  // Copy Sequence Number to buf (16 bits)
  memcpy (ptr, &icmphdr.icmp_seq, sizeof (icmphdr.icmp_seq));
  ptr += sizeof (icmphdr.icmp_seq);
  chksumlen += sizeof (icmphdr.icmp_seq);

  // Copy payload to buf
  memcpy (ptr, payload, payloadlen);
  ptr += payloadlen;
  chksumlen += payloadlen;

  // Pad to the next 16-bit boundary
  for (i=0; i<payloadlen%2; i++, ptr++) {
    *ptr = 0;
    ptr++;
    chksumlen++;
  }

  return checksum ((uint16_t *) buf, chksumlen);
}

// Computing the internet checksum (RFC 1071).
// Note that the internet checksum does not preclude collisions.
uint16_t
checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

// Allocate memory for an array of chars.
char *
allocate_strmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (char *) malloc (len * sizeof (char));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (char));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of unsigned chars.
uint8_t *
allocate_ustrmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of ints.
int *
allocate_intmem (int len)
{
  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_intmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (int *) malloc (len * sizeof (int));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (int));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_intmem().\n");
    exit (EXIT_FAILURE);
  }
}