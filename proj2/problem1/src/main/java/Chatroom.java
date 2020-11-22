import java.sql.SQLOutput;
import java.util.Collections;
import java.util.Properties;
import java.util.Scanner;
import java.util.ArrayList;

import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;

import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.ProducerConfig;
import org.apache.kafka.clients.producer.ProducerRecord;

public class Chatroom {
    public static void main(String[] args) {
        // 0: Login  1: Chatting Window 2: Chat Room
        int mode = 0;
        int option;
        boolean continue_flag;
        String str_input="", chatroom_name="", username="";
        Scanner sc = new Scanner(System.in);
        KafkaProducer<String, String> producer;
        KafkaConsumer<String, String> consumer;

        while(true) {
            /* -----------------
                LOGIN
            -------------------- */
            if (mode == 0) {
                System.out.printf("Welcome to CacaoTalk\n");
                System.out.printf("1. Log in\n");
                System.out.printf("2. Exit\n");
                System.out.println();

                continue_flag = true;
                while (continue_flag) {
                    option = cmd_get_int(sc);
                    if (1 > option || option > 2) {
                        System.out.println("Invalid Input\n");
                        continue;
                    }

                    continue_flag = false;
                    switch (option) {
                        case 1:
                            username = cmd_get_str(sc, "ID: ");
                            System.out.println();
                            mode = 1;
                            break;
                        case 2:
                            System.out.println("Bye.");
                            return;
                    }
                }
            } //mode 0
            /* ---------------------
                Chatting Window
            ------------------------ */
            else if (mode == 1) {
                System.out.println("Chatting Window");
                System.out.println("1. List");
                System.out.println("2. Make");
                System.out.println("3. Join");
                System.out.println("4. Log out");
                System.out.println();

                String room_list_name = "_" + username + "_room";
                producer = initialize_producer(username);
                consumer = initialize_consumer(username, room_list_name, "earliest");
                consumer.poll(0);
                consumer.seekToBeginning(consumer.assignment());

                continue_flag = true;
                ArrayList<String> chatrooms = new ArrayList<String>();
                while (continue_flag) {
                    option = cmd_get_int(sc);
                    if (1 > option || option > 4) {
                        System.out.printf("[%d] Invalid Input\n", option);
                        continue;
                    }

                    switch (option) {
                        case 1:
                            // Read new messages
                            ConsumerRecords<String, String> records = consumer.poll(1000);
                            for (ConsumerRecord<String, String> record : records)
                                chatrooms.add(record.value());
                            // Print list of chatroom
                            boolean is_new = false;
                            for (String name : chatrooms) {
                                System.out.println(name);
                                is_new = true;
                            }
                            if (is_new)
                                System.out.println();
                            break;
                        case 2:
                            // Make new roomname and produce message
                            str_input = cmd_get_str(sc, "Chat room name: ");
                            boolean duplicate_name = false;
                            for (String name: chatrooms)
                                if (name == str_input)
                                    duplicate_name = true;
                            if (duplicate_name)
                                System.out.println("Duplicate chatroom name!");
                            else {
                                ProducerRecord<String, String> record = new ProducerRecord<>(room_list_name, username, str_input);
                                producer.send(record);
                                System.out.printf("\"%s\" is created!\n", str_input);
                            }
                            break;
                        case 3:
                            ConsumerRecords<String, String> records_2 = consumer.poll(1000);
                            for (ConsumerRecord<String, String> record_2 : records_2)
                                chatrooms.add(record_2.value());
                            chatroom_name = cmd_get_str(sc, "Chat room name: ");
                            // Check if user has entered existing chatroom name
                            boolean is_valid = false;
                            for (String name : chatrooms)
                                if (name.equals(chatroom_name))
                                    is_valid = true;
                            if (!is_valid)
                                System.out.println("Invalid chatroom name!");
                            else {
                                System.out.println();
                                mode = 2;
                                continue_flag = false;
                            }
                            break;
                        case 4:
                            System.out.println();
                            mode = 0;
                            continue_flag = false;
                            break;
                    }
                }

                producer.close();
                consumer.close();
            } // mode 1
            /* ---------------------
                Chatting Window
            ------------------------ */
            else if (mode == 2) {
                System.out.printf("%s\n", chatroom_name);
                System.out.println("1. Read");
                System.out.println("2. Write");
                System.out.println("3. Reset");
                System.out.println("4. Exit");
                System.out.println();

                producer = initialize_producer(username);
                consumer = initialize_consumer(username, "chat_"+chatroom_name, "latest");

                continue_flag = true;
                while (continue_flag) {
                    option = cmd_get_int(sc);
                    if (1 > option || option > 4) {
                        System.out.printf("[%d] Invalid Input\n", option);
                        continue;
                    }

                    switch (option) {
                        case 1:
                            ConsumerRecords<String, String> records = consumer.poll(1000);
                            boolean is_new = false;
                            for (ConsumerRecord<String, String> record : records) {
                                System.out.printf("%s: %s\n", record.key(), record.value());
                                is_new = true;
                            }
                            if (is_new)
                                System.out.println();
                            break;
                        case 2:
                            str_input = cmd_get_str(sc, "Text: ");
                            ProducerRecord<String, String> record = new ProducerRecord<>("chat_"+chatroom_name, username, str_input);
                            producer.send(record);
                            break;
                        case 3:
                            consumer.seekToBeginning(consumer.assignment());
                            break;
                        case 4:
                            System.out.println();
                            continue_flag = false;
                            mode = 1;
                            break;
                    }
                }

                producer.close();
                consumer.close();
            }// mode 2
        }
    }

    public static int cmd_get_int(Scanner sc, String msg){
        System.out.printf("Cacaotalk> %s", msg);
        int user_input =  sc.nextInt();
        sc.nextLine();
        return user_input;
    }
    public static int cmd_get_int(Scanner sc){
        return cmd_get_int(sc, "");
    }

    public static String cmd_get_str(Scanner sc, String msg){
        System.out.printf("Cacaotalk> %s", msg);
        return sc.nextLine();
    }
    public static String cmd_get_str(Scanner sc){
        return cmd_get_str(sc, "");
    }

    public static KafkaConsumer<String, String> initialize_consumer(String username, String subscr_name, String reset_config){
        Properties config = new Properties();
        config.put(ConsumerConfig.GROUP_ID_CONFIG, username);
        config.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, "localhost:9092");
        config.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, "org.apache.kafka.common.serialization.StringDeserializer");
        config.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, "org.apache.kafka.common.serialization.StringDeserializer");
        config.put(ConsumerConfig.AUTO_OFFSET_RESET_CONFIG, "earliest");
        config.put(ConsumerConfig.ENABLE_AUTO_COMMIT_CONFIG, "true");

        KafkaConsumer<String, String> consumer = new KafkaConsumer<>(config);
        consumer.subscribe(Collections.singletonList(subscr_name));
        return consumer;
    }

    public static KafkaProducer<String, String> initialize_producer(String username){
        Properties config = new Properties();
        config.put(ProducerConfig.BOOTSTRAP_SERVERS_CONFIG, "localhost:9092");
        config.put(ProducerConfig.CLIENT_ID_CONFIG, username);
        config.put(ProducerConfig.KEY_SERIALIZER_CLASS_CONFIG, "org.apache.kafka.common.serialization.StringSerializer");
        config.put(ProducerConfig.VALUE_SERIALIZER_CLASS_CONFIG, "org.apache.kafka.common.serialization.StringSerializer");
        config.put(ProducerConfig.LINGER_MS_CONFIG, 1);
//        config.put(ProducerConfig.COMPRESSION_TYPE_CONFIG, "lz4");

        KafkaProducer<String, String> producer = new KafkaProducer<>(config);
        return producer;
    }
}
