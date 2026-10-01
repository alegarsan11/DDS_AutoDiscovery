#include <iostream>
#include <thread>
#include <chrono>

// Cabeceras de tipos core y locadores en Fast DDS v3
#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/rtps/common/Time_t.hpp>
#include <fastdds/rtps/common/Locator.hpp>
#include <fastdds/utils/IPLocator.hpp>

// Cabeceras DDS de Suscriptor
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// Cabeceras generadas por fastddsgen
#include "HelloWorld.hpp"
#include "HelloWorldPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

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
};

int main()
{
    DomainParticipantQos participant_qos;
    participant_qos.name("Suscriptor_Resilient");

    // 1. Tiempos de Discovery agresivos (5s lease, 1s anuncio)
    participant_qos.wire_protocol().builtin.discovery_config.leaseDuration = Duration_t(5, 0);
    participant_qos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(1, 0);

    // 2. Peer de rescate: Pon aquí la IP ESTÁTICA del PUBLICADOR
    // Esto permite el descubrimiento en caliente si la red no estaba activa al arrancar
    eprosima::fastdds::rtps::Locator_t remote_peer;
    eprosima::fastdds::rtps::IPLocator::setIPv4(remote_peer, "192.168.1.130"); // <--- Cambiar por IP del Publicador
    remote_peer.port = 7400; // Puerto por defecto para PDP en Dominio 0
    participant_qos.wire_protocol().builtin.initialPeersList.push_back(remote_peer);

    // Crear el participante
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    if (participant == nullptr) {
        std::cerr << "Error al crear el DomainParticipant." << std::endl;
        return 1;
    }

    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);

    Topic* topic = participant->create_topic(
        "HelloWorldTopic",
        type.get_type_name(),
        TOPIC_QOS_DEFAULT
    );

    if (topic == nullptr) {
        std::cerr << "Error al crear el Topic." << std::endl;
        return 1;
    }

    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

    SubListener listener;
    DataReader* reader = subscriber->create_datareader(
        topic,
        DATAREADER_QOS_DEFAULT,
        &listener
    );

    if (reader == nullptr) {
        std::cerr << "Error al crear el DataReader." << std::endl;
        return 1;
    }

    std::cout << "Suscriptor listo en 'HelloWorldTopic'. Presiona ENTER para salir..." << std::endl;

    std::cin.ignore();

    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}