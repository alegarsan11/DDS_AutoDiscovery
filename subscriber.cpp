#include <iostream>
#include <thread>
#include <chrono>

// Cabecera para ReturnCode_t y RETCODE_OK
#include <fastdds/dds/core/ReturnCode.hpp>

// Cabeceras de Fast DDS
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

// Listener para manejar las recepciones de datos
class SubListener : public DataReaderListener
{
public:
    SubListener() = default;
    ~SubListener() override = default;

    // Se ejecuta automáticamente cuando llega una nueva muestra
    void on_data_available(DataReader* reader) override
    {
        HelloWorld msg;
        SampleInfo info;

        // Comparación directa con RETCODE_OK
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
    // 1. Crear el DomainParticipant (Dominio 0)
    DomainParticipantQos participant_qos;
    participant_qos.name("Suscriptor_Estatico");

    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    if (participant == nullptr) {
        std::cerr << "Error al crear el DomainParticipant." << std::endl;
        return 1;
    }

    // 2. Registrar el tipo de dato
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);

    // 3. Crear el Topic
    Topic* topic = participant->create_topic(
        "HelloWorldTopic",
        type.get_type_name(),
        TOPIC_QOS_DEFAULT
    );

    if (topic == nullptr) {
        std::cerr << "Error al crear el Topic." << std::endl;
        return 1;
    }

    // 4. Crear el Subscriber
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

    // 5. Instanciar el Listener y crear el DataReader
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

    std::cout << "Suscriptor a la escucha en 'HelloWorldTopic'. Presiona ENTER para salir..." << std::endl;

    // Mantener la ejecución activa
    std::cin.ignore();

    // 6. Limpieza de recursos
    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}