#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "matrix_utils.h"
#include "training.h"


// ReLU activation function
double ReLU(double x) {
    return fmax(0, x);
};

double dReLU(double x) {
    if (x > 0) {
        return 1;
    } else {
        return 0;
    }
}

//normalized softmax function for the output layer
void *softmax(Matrix *matrix, Matrix *result) {

    //calculate sum for normalization
    double sum = 0;
    for (int i = 0; i < matrix->rows; i++) {
        sum += exp(matrix->values[i][0]);
    }

    //calculate softmax
    for (int i = 0; i < matrix->rows; i++) {
        result->values[i][0] = exp(matrix->values[i][0]) / sum;
    }
}

struct Layer {
    int index;
    int input_size;
    int layer_size;
    int output_size;
    Matrix *input;
    Matrix *weights;
    Matrix *delta_weights;
    Matrix *biases;
    Matrix *delta_biases;
    Matrix *weighted_sums;
    Matrix *activations;
    Matrix *deltas; //error of the layer
} typedef Layer;

// define network struct
struct network {
    Layer **layers;
    int number_of_layers;
} typedef Network;

// create network
Network *create_network(int number_of_layers, int *layer_sizes) {
    // allocate memory for network
    Network *network = malloc(sizeof(Network));

    network->number_of_layers = number_of_layers;
    // allocate memory for layers
    network->layers = malloc(number_of_layers * sizeof(Layer *));
    // create layers
    for (int i = 0; i < number_of_layers; i++) {
        network->layers[i] = malloc(sizeof(Layer));
        network->layers[i]->index = i;
        network->layers[i]->layer_size = layer_sizes[i];
        if (i == 0) {//input layer
            network->layers[i]->input_size = 0;
            network->layers[i]->input = create_matrix(0, 0);
        } else {
            network->layers[i]->input_size = layer_sizes[i - 1];
            network->layers[i]->input = network->layers[i - 1]->activations;
        }
        if (i == number_of_layers - 1) {//output layer
            network->layers[i]->output_size = 0;
        } else { //hidden layers
            network->layers[i]->output_size = layer_sizes[i + 1];
        }

        network->layers[i]->weights = create_matrix(network->layers[i]->layer_size, network->layers[i]->input_size);
        network->layers[i]->delta_weights = create_matrix(network->layers[i]->layer_size,
                                                          network->layers[i]->input_size);
        network->layers[i]->weighted_sums = create_matrix(network->layers[i]->layer_size, 1);
        network->layers[i]->activations = create_matrix(network->layers[i]->layer_size, 1);
        network->layers[i]->deltas = create_matrix(network->layers[i]->layer_size, 1);

        //randomize weights
        randomize_matrix(network->layers[i]->weights);

        //initialize bias
        network->layers[i]->biases = create_matrix(network->layers[i]->layer_size, 1);
        network->layers[i]->delta_biases = create_matrix(network->layers[i]->layer_size, 1);
        randomize_matrix(network->layers[i]->biases);
    }
    return network;
}

void free_network(Network *network) {
    for (int i = 0; i < network->number_of_layers; i++) {
        free_matrix(network->layers[i]->weights);
        free_matrix(network->layers[i]->delta_weights);
        free_matrix(network->layers[i]->biases);
        free_matrix(network->layers[i]->delta_biases);
        free_matrix(network->layers[i]->weighted_sums);
        free_matrix(network->layers[i]->activations);
        free_matrix(network->layers[i]->deltas);
        free(network->layers[i]);
    }
    free(network->layers);
    free(network);
}

// propagate forward through the network
void propagate_forward(Network *network, Matrix *input) {
    //assign input to the activations of the input layer
    network->layers[0]->activations->values[0][0] = input->values[0][0];
    network->layers[0]->activations->values[1][0] = input->values[1][0];
    network->layers[0]->activations->values[2][0] = input->values[2][0];

    //calculate weighted sums and activations for hidden layers and output layer (exclude input layer i=1)
    for (int i = 1; i < network->number_of_layers - 1; i++) {
        //calculate weighted sums for hidden layers
        matrix_multiply(network->layers[i]->weights, network->layers[i]->input, network->layers[i]->weighted_sums);
        //add bias
        matrix_add(network->layers[i]->weighted_sums, network->layers[i]->biases,
                   network->layers[i]->weighted_sums);

        //calculate activations
        matrix_apply_function(network->layers[i]->weighted_sums, ReLU, network->layers[i]->activations);
        //set activations as input for next layer
        network->layers[i + 1]->input = network->layers[i]->activations;
    }
    //calculate output layer weighted sums
    matrix_multiply(network->layers[network->number_of_layers - 1]->weights,
                    network->layers[network->number_of_layers - 1]->input,
                    network->layers[network->number_of_layers - 1]->weighted_sums);
    //add bias
    matrix_add(network->layers[network->number_of_layers - 1]->weighted_sums,
               network->layers[network->number_of_layers - 1]->biases,
               network->layers[network->number_of_layers - 1]->weighted_sums);
    //apply softmax
    softmax(network->layers[network->number_of_layers - 1]->weighted_sums,
            network->layers[network->number_of_layers - 1]->activations);
}

// print the entire structure of the network
void print_network(Network *network) {
    for (int i = 0; i < network->number_of_layers; i++) {
        printf("Layer %d\n", i);
        printf("Input size: %d\n", network->layers[i]->input_size);
        printf("Layer size: %d\n", network->layers[i]->layer_size);
        printf("Output size: %d\n", network->layers[i]->output_size);
        printf("Input:\n");
        print_matrix(network->layers[i]->input);
        printf("Weights:\n");
        print_matrix(network->layers[i]->weights);
        printf("Weighted sums:\n");
        print_matrix(network->layers[i]->weighted_sums);
        printf("Activations:\n");
        print_matrix(network->layers[i]->activations);
        printf("\n");
    }
}

double calculate_loss(Matrix *output_layer, Matrix *target) {
    double loss = 0;
    for (int i = 0; i < output_layer->rows; i++) {
        loss += pow(output_layer->values[i][0] - target->values[i][0], 2);
    }
    return loss;
}

double calculate_average_loss(Network *network, TrainingDataPacket **training_data, int length_of_training_data) {
    double loss = 0;
    for (int j = 0; j < length_of_training_data; j++) {
        //propagate forward
        propagate_forward(network, training_data[j]->input);
        //calculate loss
        loss += calculate_loss(network->layers[network->number_of_layers - 1]->activations, training_data[j]->target);
    }
    //return average loss
    return loss / length_of_training_data;
}


//calculate average success rate of the network on the training values
double
calculate_average_success_rate(Network *network, TrainingDataPacket **training_data, int length_of_training_data) {
    double success_rate = 0;
    for (int j = 0; j < length_of_training_data; j++) {
        //propagate forward
        propagate_forward(network, training_data[j]->input);
        //check if the output matches the target
        if (vector_max_index(network->layers[network->number_of_layers - 1]->activations) ==
            vector_max_index(training_data[j]->target)) {
            success_rate++;
        }
    }
    return success_rate / length_of_training_data;
}

double output_node_cost_derivative(double output, double target) {
    return 2 * (output - target);
}

double ReLU_derivative(double x) {
    if (x > 0) {
        return 1;
    } else {
        return 0;
    }
}

//calculate the intermediate values used for gradient descent (da_n/dz_n*dl/da_n) values for all neurons in a layer
void calculate_deltas_for_layer(Network *network, int layer_index, Matrix *target) {
    //calculate deltas for output layer
    if (layer_index == network->number_of_layers - 1) {
        for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
            network->layers[layer_index]->deltas->values[i][0] = output_node_cost_derivative(
                    network->layers[layer_index]->activations->values[i][0], target->values[i][0]) *
                                                                 ReLU_derivative(
                                                                         network->layers[layer_index]->weighted_sums->values[i][0]);
        }
    } else {
        //calculate deltas for hidden layers
        for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
            double sum = 0;
            for (int j = 0; j < network->layers[layer_index + 1]->layer_size; j++) {
                sum += network->layers[layer_index + 1]->weights->values[j][i] *
                       network->layers[layer_index + 1]->deltas->values[j][0];
            }
            network->layers[layer_index]->deltas->values[i][0] = sum *
                                                                 ReLU_derivative(
                                                                         network->layers[layer_index]->weighted_sums->values[i][0]);
        }
    }
}

void calculate_delta_weights_for_layer(Network *network, int layer_index) {
    for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
        for (int j = 0; j < network->layers[layer_index]->input_size; j++) {
            network->layers[layer_index]->delta_weights->values[i][j] +=
                    network->layers[layer_index]->deltas->values[i][0] *
                    network->layers[layer_index]->input->values[j][0];
        }
    }
}

void calculate_delta_biases_for_layer(Network *network, int layer_index) {
    for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
        network->layers[layer_index]->delta_biases->values[i][0] += network->layers[layer_index]->deltas->values[i][0];
    }
}

void average_delta_weights_for_layer(Network *network, int layer_index, int length_of_training_data) {
    for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
        for (int j = 0; j < network->layers[layer_index]->input_size; j++) {
            network->layers[layer_index]->delta_weights->values[i][j] /= length_of_training_data;
        }
    }
}

void average_delta_biases_for_layer(Network *network, int layer_index, int length_of_training_data) {
    for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
        network->layers[layer_index]->delta_biases->values[i][0] /= length_of_training_data;
    }
}

void update_weights_for_layer(Network *network, int layer_index, double learning_rate) {
    for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
        for (int j = 0; j < network->layers[layer_index]->input_size; j++) {
            network->layers[layer_index]->weights->values[i][j] -=
                    learning_rate * network->layers[layer_index]->delta_weights->values[i][j];
        }
    }
}

void update_biases_for_layer(Network *network, int layer_index, double learning_rate) {
    for (int i = 0; i < network->layers[layer_index]->layer_size; i++) {
        network->layers[layer_index]->biases->values[i][0] -=
                learning_rate * network->layers[layer_index]->delta_biases->values[i][0];
    }
}

void train_network(Network *network, TrainingDataPacket **training_data, int length_of_training_data, int epochs,
                   double learning_rate) {
    for (int i = 0; i < epochs; i++) {
        for (int j = 0; j < length_of_training_data; j++) {
            //propagate forward
            propagate_forward(network, training_data[j]->input);
            //calculate deltas for output layer
            calculate_deltas_for_layer(network, network->number_of_layers - 1, training_data[j]->target);
            //calculate deltas for hidden layers
            for (int k = network->number_of_layers - 2; k >= 0; k--) {
                calculate_deltas_for_layer(network, k, training_data[j]->target);
            }
            //calculate delta weights for output layer
            calculate_delta_weights_for_layer(network, network->number_of_layers - 1);
            //calculate delta weights for hidden layers
            for (int k = network->number_of_layers - 2; k >= 0; k--) {
                calculate_delta_weights_for_layer(network, k);
            }
            //calculate delta biases for output layer
            calculate_delta_biases_for_layer(network, network->number_of_layers - 1);
            //calculate delta biases for hidden layers
            for (int k = network->number_of_layers - 2; k >= 0; k--) {
                calculate_delta_biases_for_layer(network, k);
            }
        }
        //average delta weights for output layer
        average_delta_weights_for_layer(network, network->number_of_layers - 1, length_of_training_data);
        //average delta weights for hidden layers
        for (int k = network->number_of_layers - 2; k >= 0; k--) {
            average_delta_weights_for_layer(network, k, length_of_training_data);
        }
        //average delta biases for output layer
        average_delta_biases_for_layer(network, network->number_of_layers - 1, length_of_training_data);
        //average delta biases for hidden layers
        for (int k = network->number_of_layers - 2; k >= 0; k--) {
            average_delta_biases_for_layer(network, k, length_of_training_data);
        }
        //update weights for output layer
        update_weights_for_layer(network, network->number_of_layers - 1, learning_rate);
        //update weights for hidden layers
        for (int k = network->number_of_layers - 2; k >= 0; k--) {
            update_weights_for_layer(network, k, learning_rate);
        }
        //update biases for output layer
        update_biases_for_layer(network, network->number_of_layers - 1, learning_rate);
        //update biases for hidden layers
        for (int k = network->number_of_layers - 2; k >= 0; k--) {
            update_biases_for_layer(network, k, learning_rate);
        }
        //calculate average loss every 10 epochs
        if (i % 10 == 0) {
            double loss = calculate_average_loss(network, training_data, length_of_training_data);
            printf("avg loss: %f\n", loss);
            double success_rate = calculate_average_success_rate(network, training_data, length_of_training_data);
            printf("success rate: %f\n", success_rate);
        }
    }
}


int main() {
    srand(time(NULL));

    Network *network = create_network(4, (int[]) {3, 10, 20, 16});

    TrainingDataPacket **training_data = read_training_data(
            "C:\\Users\\szymc\\CLionProjects\\Sem2Lab2\\training_data.txt",
            60000);

    train_network(network, training_data, 60000, 100, 0.2);

    //get input from user
    Matrix *input = create_matrix(3, 1);
    printf("Enter 3 numbers: ");
    scanf("%lf %lf %lf", &input->values[0][0], &input->values[1][0], &input->values[2][0]);
    propagate_forward(network, input);
    free_matrix(input);

    //print the output
    print_matrix(network->layers[network->number_of_layers - 1]->activations);
    //print the index of the largest value
    printf("index of the largest value: %d\n",
           vector_max_index(network->layers[network->number_of_layers - 1]->activations));

    return 0;
}
//todo zapisywanie wag i biasów do pliku
//todo

// 10 neuronów wejściowych
// rzędy w kalawieturze
// połówki alfabetu


// klasyfikator kolorów
// 3 nauronowe wejście jako rgb kolor
// klasyfikacja na 12 kolorów

//zwalniaj pamięć