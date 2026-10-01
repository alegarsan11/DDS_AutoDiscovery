#include <iostream>
#include <thread>
#include <chrono>

// Cabeceras de tipos core y locadores en Fast DDS v3
#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/rtps/common/Time_t.hpp>
#include <fastdds/rtps/common/Locator.hpp>
#include <fastdds/utils/IPLocator.hpp>

// Cabeceras DDS de Publicador
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// Cabeceras generadas por fastddsgen
#include "HelloWorld.hpp"
#include "HelloWorldPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

int main()
{
    DomainParticipantQos participant_qos;
    participant_qos.name("Publicador_Resilient");

    // 1. Tiempos de Discovery agresivos (5s lease, 1s anuncio)
    participant_qos.wire_protocol().builtin.discovery_config.leaseDuration = Duration_t(5, 0);
    participant_qos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(1, 0);

    // 2. Peer de rescate: Pon aquí la IP ESTÁTICA del SUSCRIPTOR
    // Esto permite el descubrimiento en caliente si la red no estaba activa al arrancar
    eprosima::fastdds::rtps::Locator_t remote_peer;
    eprosima::fastdds::rtps::IPLocator::setIPv4(remote_peer, "192.168.1.135"); // <--- Cambiar por IP del Suscriptor
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

    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);
    DataWriter* writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT);

    if (writer == nullptr) {
        std::cerr << "Error al crear el DataWriter." << std::endl;
        return 1;
    }

    std::cout << "Publicador listo y transmitiendo en 'HelloWorldTopic'..." << std::endl;

    HelloWorld msg;
    msg.index(0);
    msg.message("Hola desde Arch Linux!");

    while (true)
    {
        msg.index(msg.index() + 1);
        writer->write(&msg);
        std::cout << "[ENVIADO] -> Index: " << msg.index() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}