#include <string>
#include <cstring>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

// Comprueba si UNA interfaz específica (ej: "wlan0") está UP y devuelve su IP
bool is_specific_interface_ready(const std::string& target_iface, std::string& out_ip)
{
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
    {
        return false;
    }

    bool ready = false;
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr) continue;

        // Comprobamos la familia IPv4 y que el nombre de la interfaz coincida
        if (ifa->ifa_addr->sa_family == AF_INET && target_iface == ifa->ifa_name)
        {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);

            bool is_up = (ifa->ifa_flags & IFF_UP) != 0;
            bool is_running = (ifa->ifa_flags & IFF_RUNNING) != 0;

            if (is_up && is_running && sa->sin_addr.s_addr != 0)
            {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sa->sin_addr), ip_str, INET_ADDRSTRLEN);
                out_ip = std::string(ip_str);
                ready = true;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ready;
}

#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

// ... dentro del bucle principal en main() ...

std::string current_ip;
const std::string target_interface = "wlan0"; // <--- Cambia esto por tu interfaz (eth0, wlan0, etc.)

while (!is_specific_interface_ready(target_interface, current_ip))
{
    std::cout << "[RED DOWN] Esperando a que " << target_interface << " esté activa..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

std::cout << "[RED UP] Interfaz " << target_interface << " lista con IP: " << current_ip << std::endl;

DomainParticipantQos participant_qos;
participant_qos.name("Publicador_Resilient");

// --- RESTRICCIÓN DE INTERFAZ EN FAST DDS ---
// 1. Desactivamos los transportes automáticos por defecto
participant_qos.transport().use_builtin_transports = false;

// 2. Creamos un transporte UDPv4 personalizado apuntando únicamente a la IP de la interfaz
auto custom_udp_transport = std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();
custom_udp_transport->interface_whitelist.push_back(current_ip); // Solo escuchará/enviará por la IP de target_interface

// 3. Añadimos el transporte a las QoS
participant_qos.transport().user_transports.push_back(custom_udp_transport);
// -------------------------------------------

// Continuar creando el DomainParticipant...