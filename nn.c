#include<stdio.h>
#include<stdlib.h>
#include <Python.h>
#include<string.h>
#include<math.h>
#include "dense_layer.h"
#include "activation_layer.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))

struct DenseLayer layers[100];
struct ActivationLayer alayers[100];


double mse(double y_true, double y_pred){
    return pow(y_true-y_pred, 2);
}

double mse_prime(double y_true, double y_pred){
    return 2*(y_pred-y_true);
}


struct Network{
    int num_layers;
};

void init_Network(struct Network *NN){
    NN->num_layers = 0;
}

void addlayer(struct Network *NN, int input_size, int output_size){
    struct DenseLayer new_layer;
    struct ActivationLayer new_alayer;
    init_ActivationLayer(&new_alayer, "tanh", "tanh_prime");
    init_DenseLayer(&new_layer, input_size, output_size);
    layers[NN->num_layers] = new_layer;
    alayers[NN->num_layers] = new_alayer;
    NN->num_layers++;
}

void fit(struct Network *NN, double X_train[][3], double y_train[],int epochs,int learning_rate){
    int samples = 3;
    for(int eno=0;eno<epochs;eno++){
        double err = 0;
        for(int i=0;i<samples;i++){
            double output[1000] = {0};    
            for(int j=0;j<(int)ARRAYSIZE(X_train[0]);j++){
                output[i] = X_train[i][j];
            }
            for(int j=0;j<NN->num_layers;j++){
                double *op = forward_propogation(&layers[j], output);
                double *op1 = act_forward_propogation(&alayers[j], op, layers[j].output_size);
                for(int k=0; k< layers[j].output_size; k++){
                    output[k] = op1[k];
                }
                free(op);
                free(op1);
            } 
            err += mse(y_train[i], output[0]);
            double error = mse_prime(y_train[i], output[0]);
            double aerror[100];
            aerror[0] = error;
            for(int j=NN->num_layers-1;j>=0;j--){
                double *op2 = act_backward_propogation(&alayers[j], aerror, layers[j].output_size ,0.001);
                double *op3 = backward_probogation(&layers[j], op2, 0.1);
                for(int k=0; k< layers[j].output_size; k++){
                    aerror[k] = op3[k];
                }
                free(op2);
                free(op3);
            }
        }
        err /= samples;
        printf("Epoch %d/%d  Error=%g\n",eno+1,epochs,err);
    }
}

double *predict(struct Network *NN, double X_train[][3]){
    int samples = 3;
    double *outputs = (double *)malloc(sizeof(double *) * 1000);
    int counter=0;
        for(int i=0;i<samples;i++){
            double output[1000] = {0};    
            for(int j=0;j<(int)ARRAYSIZE(X_train[0]);j++){
                output[i] = X_train[i][j];
            }

            for(int j=0;j<NN->num_layers;j++){
                double *op = forward_propogation(&layers[j], output);
                double *op1 = act_forward_propogation(&alayers[j], op, layers[j].output_size);
                for(int k=0; k< layers[j].output_size; k++){
                    output[k] = op1[k];
                }
                free(op);
                free(op1);
            } 
            outputs[counter] = output[0];
            counter++;
        }
    return outputs;
}


struct Network network;


static PyObject *model(PyObject *self, PyObject *args) {
    PyObject *int_list;
    int len;
    double *arr;

    if(!PyArg_ParseTuple(args, "O", &int_list)) {
        return NULL;
    }

    len = PyObject_Length(int_list);
    if (len < 0) return NULL;

    arr = (double *)malloc(sizeof(double *) * len);
    if (arr == NULL) return NULL;

    for(int i = 0;i < len; i++){
        PyObject *e;
        e = PyList_GetItem(int_list, i);
        if (!PyFloat_Check(e)) arr[i] = 0.0;
        arr[i] = PyFloat_AsDouble(e);
    }


    init_Network(&network);

    for(int i=0;i<len;i+=2){
        addlayer(&network, arr[i], arr[i+1]);
    }
    for(int i=0;i<network.num_layers;i++){
        printf("layer no. %d INP: %d OP: %d\n", i+1, layers[i].input_size, layers[i].output_size);
    }
    Py_RETURN_NONE;
}

static PyObject *mfit(PyObject *self, PyObject *args) {
    PyObject *X_train, *y_train;
    int xr, xc, epochs;
    double lr;
    double xarr[100][3];
    double yarr[100];

    if(!PyArg_ParseTuple(args, "OOid", &X_train, &y_train, &epochs, &lr)) {
        return NULL;
    }

    xr = PyObject_Length(X_train);
    PyObject *temp = PyList_GetItem(X_train, 0);
    xc = PyObject_Length(temp);

    for(int i = 0;i < xr; i++){
        PyObject *e, *e1;
        e = PyList_GetItem(X_train, i);
        for(int j=0;j<xc;j++){
            xarr[i][j] = PyFloat_AsDouble(PyList_GetItem(e, j));
        }
        e1 = PyList_GetItem(y_train, i);
        yarr[i] = PyFloat_AsDouble(e1);
    }

    fit(&network, xarr, yarr, epochs, lr);
    double *predictions = predict(&network, xarr);
    printf("\nPredictions: \n");
    for(int i=0;i<3;i++){
        printf("%g ", predictions[i]);
    }
    printf("\n");
    Py_RETURN_NONE;
}



static PyMethodDef Methods[] = {
    {"model", model, METH_VARARGS, "neural network initalisation"},
    {"fit", mfit, METH_VARARGS, "neural network training"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef tachnet = {
    PyModuleDef_HEAD_INIT,
    "tachnet",
    "Astronomically fast deep learning library",
    -1,
    Methods
};

PyMODINIT_FUNC PyInit_tachnet(void) {
    return PyModule_Create(&tachnet);
}