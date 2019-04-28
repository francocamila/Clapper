#include <msp430.h> 

//definições
#define lampada BIT6
#define saidas (BIT6+BIT0)
#define sensibilidade 8

//variáveis globais
    int contador_amostras=1;
    int soma=0;
    int media;
    int leitura;

//rotinas e funções
int modulo (int valor){       //retorna o valor absoluto de uma entrada inteira
    if (valor < 0){           // se o valor for negativo seu sinal será invertido
         valor ^= 0xffff;      // complemento de 2: inversão dos bits
         valor++;            // complemento de 2: soma 1
        }
return valor;            //se o sinal for positivo nada precisa ser feito
}

void atraso (unsigned int tempo_ms){
    TACCR0 = 999;                    // serão feitas 1000 contagens de 1 microsegundo por "tempo_ms" vezes
    TA0CTL |= MC_1;                  // timer A0 é ligado no modo up
    while (tempo_ms>0){              // laço de repetição para as "tempo_ms" contagens
        if ((TA0CTL & TAIFG)==1){    // reconhece quando a contagem terminou
            tempo_ms--;
            TA0CTL &= ~TAIFG;        //zera o TAIF para novo ciclo
        }
    }
    TA0CTL |= MC_0;            //timer A0 é desligado
    TA0CTL |= TACLR;           // TAR é zerado
    TA0CTL = TASSEL_2 + ID_0 + MC_0; // volta as configurações iniciais
}

void segunda_palma (unsigned int tempo_ms,unsigned int hold_media){  //função para identicar segunda palma
    int diferenca;

    TACCR0 = 999;                    // serão feitas 1000 contagens de 1 microsegundo por "tempo_ms" vezes
    TA0CTL |= MC_1;                  // timer A0 é ligado no modo up

    while (tempo_ms>0){              // laço de repetição para as "tempo_ms" contagens
        if ((TA0CTL & TAIFG)==1){    // reconhece quando a contagem terminou
            tempo_ms--;
            TA0CTL &= ~TAIFG;        //zera o TAIF para novo ciclo
        }
        diferenca = hold_media - leitura;//calcula a diferença entre a média e a leitura atual
        diferenca = modulo (diferenca);
        if ((sensibilidade*diferenca) > hold_media){ //verifica se houve a segunda palma
          P1OUT ^= lampada;                     //muda o estado da lâmpada
          break;                                // sai do laço se houver 2a palma
          }
    }
    TA0CTL |= MC_0;            //timer A0 é desligado
    TA0CTL |= TACLR;           // TAR é zerado
    TA0CTL = TASSEL_2 + ID_0 + MC_0; // volta as configurações iniciais
}

#pragma vector = ADC10_VECTOR              //função de interrupção do conversor A/D
__interrupt void Interrupcao_conversor_a_d(void){
    leitura = ADC10MEM;
    if(contador_amostras==16){
        media = soma + ADC10MEM;
        media = (media/16);
        contador_amostras = 1;
        soma = 0;
//        P1OUT &= ~BIT0;
    }
    else{
        soma += ADC10MEM;
        contador_amostras++;
 //       P1OUT |= BIT0;
    }
    ADC10CTL0 |= ENC + ADC10SC; // inicia a próxima conversao
}

int main(void) {
    int hold_media;
    int diferenca=0;

    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer
    __bis_SR_register(GIE);     //interrupções habilitadas

//Configuração do clock
    BCSCTL1 = CALBC1_1MHZ;      //MCLK e SMCLK @ 1MHz
    DCOCTL = CALDCO_1MHZ;       //MCLK e SMCLK @ 1MHz

//Configuração do timer A0
    TA0CTL = TASSEL_2 + ID_0 + MC_0;   //inicialmente o timer A0 ficará desligado (MC_0)

//Configuração do ADC10

    ADC10CTL1 |= INCH_5 // configura entrada 5 do ADC
    + SHS_0 // sampleandhold controlado pelo bit
    // ADC10SC
    + ADC10DIV_7 // seleciona divisao do clock por 8
    + ADC10SSEL_0 // seleciona interno
    + CONSEQ_0; // seleciona modo 0

    ADC10CTL0 = SREF_0 // seleciona Vcc como VR+ e GND como VR+
    +ADC10SHT_0 // tempo de sampleandhold de 4 ciclos de clock
    + ADC10ON // ativa o canversor
    + ADC10IE; // habilita interrupcao do ADC10

    ADC10CTL0 |= ENC + ADC10SC; // inicia a conversao
 //   ADC10DTC1 = 1;

// Configuração dos pinos PORT1
     P1DIR |= saidas;                       // lâmpada como saída
     P1OUT &= ~ saidas;                      // lâmpada inicialmente apagada

// ************** FIM DA CONFIGURAÇÃO INICIAL **************
     while(1){
     diferenca = media - leitura;//calcula a diferença entre a média e a leitura atual
     diferenca = modulo (diferenca);
     
     hold_media = media;
     if ((sensibilidade*diferenca) > hold_media){  //verifica se houve a primeira palma
        P1OUT ^= BIT0;
        atraso (60);
        segunda_palma(500,hold_media);
        atraso(60);
     }
     }
     return 0;
}
