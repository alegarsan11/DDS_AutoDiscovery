#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

// Cabeceras de tipos core y transporte en Fast DDS v3
#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/rtps/common/Time_t.hpp>
#include <fastdds/rtps/common/Locator.hpp>
#include <fastdds/utils/IPLocator.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.hpp>

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
    participant_qos.name("Suscriptor_Resilient_Multicast");

    // 1. Reconfigurar el transporte UDPv4 explícito para escuchar en 0.0.0.0
    participant_qos.transport().use_builtin_transports = false;
    auto udp_transport = std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();
    participant_qos.transport().user_transports.push_back(udp_transport);

    // 2. Configurar tiempos de Discovery agresivos (5s de lease, 1s de anuncio)
    participant_qos.wire_protocol().builtin.discovery_config.leaseDuration = Duration_t(5, 0);
    participant_qos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(1, 0);

    // 3. Forzar el registro del grupo Multicast RTPS estándar (239.255.0.1:7400 para Dominio 0)
    eprosima::fastdds::rtps::Locator_t multicast_locator;
    eprosima::fastdds::rtps::IPLocator::setIPv4(multicast_locator, "239.255.0.1");
    multicast_locator.port = 7400;

    participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.push_back(multicast_locator);

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

    std::cout << "Suscriptor listo en 'HelloWorldTopic'. Esperando conexion... Presiona ENTER para salir." << std::endl;

    std::cin.ignore();

    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}