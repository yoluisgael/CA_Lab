# CA_Lab

**CA_Lab** es una aplicación para experimentar con las distintas reglas de los Autómatas Celulares Elementales.
## Características

*   Visualización de la evolución del estado inicial de una regla.
*   Interfaz gráfica intuitiva para configurar parámetros.
*   Estados inicial y final guardables y cargables.

![Aplicación](https://github.com/yoluisgael/CA_Lab/blob/main/CA_Lab.png)

## Cómo usar

1.  Clona el repositorio.
2.  Compila el código.
3.  Crear directorios necesarios de la sección de [Directorios](#directorios). 
4.  Ejecuta el programa.
5.  Configura el estado inicial ya sea con los botones del menú o clickeando el arreglo inicial (Hasta arriba de la pantalla).
6.  Presiona [Enter] para procesar el autómata celular.

## Controles

*   Clic izquierdo: Dibujar cambiar estado de celdas en el arreglo inicial (Hasta arriba).
*   Enter: Procesar autómata celular seleccionado.
*   F1: Procesar todas las reglas no-equivalentes de autómatas celulares para el mismo arreglo inicial.
*   Tecla Esc: Abrir menú.

## Gráficas
Este programa puede graficar las siguientes métricas del autómata celular procesado:
1. Densidad
2. Log10 de la densidad
3. Entropía
4. Media
5. Varianza

## Directorios
* Configuraciones para cargar: C:\CA\saves
* Textos para hacer plots: C:\CA\plots
* Imágenes de plots: C:\CA\graficas
* Renders de CA's: C:\CA\renders

## Dependencias

*   SFML
*   gnuplot

## Contribuciones

Las contribuciones son bienvenidas. Crea un pull request con tus cambios.

## Licencia

Este proyecto está licenciado bajo la Licencia MIT.
