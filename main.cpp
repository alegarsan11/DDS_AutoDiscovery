#include <iostream>
#include <thread>
#include <chrono>

// Cabeceras de Fast DDS
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

// Cabecera generada a partir del IDL
#include "HelloWorldPubSubTypes.hpp"

using namespace eprosima::fastdds::dds;

int main()
{
    // 1. Crear el DomainParticipant
    DomainParticipantQos participant_qos;
    participant_qos.name("Publicador_Estatico");

    DomainParticipant* participant = 
        DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    if (participant == nullptr) {
        std::cerr << "Error al crear el DomainParticipant." << std::endl;
        return 1;
    }

    // 2. Registrar el tipo de dato generado
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

    // 4. Crear el Publisher y DataWriter
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);
    DataWriter* writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT);

    if (writer == nullptr) {
        std::cerr << "Error al crear el DataWriter." << std::endl;
        return 1;
    }

    std::cout << "Publicador iniciado en el topic 'HelloWorldTopic'. Enviando datos..." << std::endl;

    // 5. Instanciar la estructura y bucle de envío
    HelloWorld st;
    uint32_t count = 0;

    while (true) {
        count++;
        st.index(count);
        st.message("Hola desde Fast DDS!");

        writer->write(&st);

        std::cout << "Publicado -> Index: " << st.index() 
                  << " | Mensaje: " << st.message() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 6. Limpieza
    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);

    return 0;
}