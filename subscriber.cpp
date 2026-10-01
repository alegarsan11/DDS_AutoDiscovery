#include <iostream>
#include <thread>
#include <chrono>

// Cabeceras POSIX para comprobar interfaces de red en Linux
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

// Cabeceras Fast DDS v3
#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/rtps/common/Time_t.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "HelloWorld.hpp"
#include "HelloWorldPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

// Comprueba si existe al menos una interfaz física (no loopback) lista y con IP
bool is_network_ready()
{
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
    {
        return false;
    }

    bool ready = false;
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
        {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            uint32_t ip = ntohl(sa->sin_addr.s_addr);

            bool is_loopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
            bool is_up = (ifa->ifa_flags & IFF_UP) != 0;
            bool is_running = (ifa->ifa_flags & IFF_RUNNING) != 0;

            // IP válida (no 127.x.x.x ni 0.0.0.0) y tarjeta activa con carrier
            if (is_up && is_running && !is_loopback && ip != 0 && (ip & 0xFF000000) != 0x7F000000)
            {
                ready = true;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ready;
}

class SubListener : public DataReaderListener
{
public:
    SubListener() = default;
    ~SubListener() override = default;

    void on_data_available(DataReader* reader) override
    {
        HelloWorld msg;
        SampleInfo info;

        if (reader->take_next_sample(&msg, &info) == RETCODE_OK)
        {
            if (info.valid_data)
            {
                std::cout << "[RECIBIDO] -> Index: " << msg.index()
                          << " | Mensaje: " << msg.message() << std::endl;
            }
        }
    }

    void on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info) override
    {
        (void)reader;
        if (info.current_count_change == 1)
        {
            std::cout << "\n[DISCOVERY] >>> Publicador detectado y conectado! <<<\n" << std::endl;
        }
        else if (info.current_count_change == -1)
        {
            std::cout << "\n[DISCOVERY] <<< Publicador desconectado. <<<\n" << std::endl;
        }
    }
};

int main()
{
    std::cout << "=== Suscriptor Resiliente DDS (Radio 2.4GHz) ===" << std::endl;

    while (true)
    {
        // 1. Bucle de espera si la red o el enlace radio no están listos
        while (!is_network_ready())
        {
            std::cout << "[RED DOWN] Esperando a que el enlace radio de 2.4 GHz esté activo..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        std::cout << "[RED UP] Interfaz de red activa detectada. Inicializando Fast DDS..." << std::endl;

        DomainParticipantQos participant_qos;
        participant_qos.name("Suscriptor_Resilient");

        // Discovery agresivo para rápida reconexión tras microcortes
        participant_qos.wire_protocol().builtin.discovery_config.leaseDuration = Duration_t(5, 0);
        participant_qos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(1, 0);

        DomainParticipant* participant =
            DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

        if (participant == nullptr) {
            std::cerr << "Error al crear el DomainParticipant. Reintentando..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        TypeSupport type(new HelloWorldPubSubType());
        type.register_type(participant);

        Topic* topic = participant->create_topic(
            "HelloWorldTopic",
            type.get_type_name(),
            TOPIC_QOS_DEFAULT
        );

        Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

        SubListener listener;
        DataReader* reader = subscriber->create_datareader(
            topic,
            DATAREADER_QOS_DEFAULT,
            &listener
        );

        std::cout << ">>> Suscriptor LISTO y a la escucha. Monitoreando conexión... <<<" << std::endl;

        // 2. Monitoreo continuo de la red mientras recibe mensajes
        while (is_network_ready())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 3. Si la red cae, destruimos entidades para re-inicializar al reconectar
        std::cout << "\n[ALERTA] Caída de interfaz/enlace detectada!" << std::endl;
        std::cout << "Liberando recursos de Fast DDS para esperar nueva conexión..." << std::endl;

        participant->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(participant);
    }

    return 0;
}