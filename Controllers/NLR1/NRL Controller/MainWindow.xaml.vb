Imports System.IO.Ports
Imports System.Runtime.InteropServices
Imports System.Threading


Public Enum NLRStatus
    CONNECTED
    DISCONNECTED
    UNKNOWED
End Enum

Public Class RocketData
    Public Latitude As Single
    Public Longitude As Single
    Public Altitude As UShort
    Public Speed As UShort
    Public Satellites As Byte
    Public GpsAvailable As Boolean
    Public LoraConnected As Boolean
    Public BmpStatus As Boolean
    Public Pressure As UShort       ' hPa * 10
    Public Temperature As Short     ' °C * 10
    Public PressureAltitude As Short ' m * 10
End Class

Public Class MainWindow
    <DllImport("kernel32.dll")>
    Public Shared Function AllocConsole() As Boolean
    End Function

    Private serialPort As SerialPort
    Private openPortThread As Thread
    Private isPortOpening As Boolean = False

    Public Sub New()
        InitializeComponent()
        AllocConsole()
        Console.WriteLine("Console started !")
        ListAvailablePorts()
        UpdateStatus(NLRStatus.DISCONNECTED)
    End Sub

    Private Sub ListAvailablePorts()
        ' Obtenir les noms des ports COM disponibles
        Dim availablePorts = SerialPort.GetPortNames()
        Console.WriteLine("Available ports: " & String.Join(", ", availablePorts))
        ' Afficher les ports dans une ComboBox
        ComboBoxPorts.Items.Clear()
        For Each port In availablePorts
            ComboBoxPorts.Items.Add(port)
        Next
    End Sub

    Private Function DecodeRocketData(buffer() As Byte) As RocketData
        Dim data As New RocketData()
        Dim i As Integer = 0

        data.Latitude = BitConverter.ToSingle(buffer, i) : i += 4
        data.Longitude = BitConverter.ToSingle(buffer, i) : i += 4
        data.Altitude = BitConverter.ToUInt16(buffer, i) : i += 2
        data.Speed = BitConverter.ToUInt16(buffer, i) : i += 2
        data.Satellites = buffer(i) : i += 1
        data.GpsAvailable = (buffer(i) <> 0) : i += 1
        data.LoraConnected = (buffer(i) <> 0) : i += 1
        data.BmpStatus = (buffer(i) <> 0) : i += 1
        data.Pressure = BitConverter.ToUInt16(buffer, i) : i += 2
        data.Temperature = BitConverter.ToInt16(buffer, i) : i += 2
        data.PressureAltitude = BitConverter.ToInt16(buffer, i)

        Return data
    End Function


    Private Sub TryOpenPort(port As String)
        While isPortOpening
            Try
                serialPort = New SerialPort(port, 9600, Parity.None, 8, StopBits.One)
                serialPort.Open()

                ' Si le port s'ouvre avec succès, arrêter la boucle
                isPortOpening = False
                UpdateStatus(NLRStatus.CONNECTED)

                Console.WriteLine("Port COM ouvert : " & port & Environment.NewLine)
                AddHandler serialPort.DataReceived, AddressOf DataReceivedHandler
                StartPortMonitor()
                Exit While

            Catch ex As Exception
                ' Afficher un message d'erreur ou journaliser l'erreur
                UpdateStatus(NLRStatus.DISCONNECTED)
                Console.WriteLine("Erreur lors de l'ouverture du port COM : " & ex.Message)
                ' Attendre avant de réessayer
                Thread.Sleep(500)
            End Try
        End While
    End Sub

    Private Sub ConnectionClose()
        UpdateStatus(NLRStatus.DISCONNECTED)
    End Sub

    Private Sub StartPortMonitor()
        Task.Run(Sub()
                     While serialPort.IsOpen
                         Try
                             Dim buffer = serialPort.BytesToRead
                         Catch ex As Exception
                             Exit While
                         End Try
                         Threading.Thread.Sleep(500)
                     End While
                     Dispatcher.BeginInvoke(Sub()
                                                ConnectionClose()
                                                serialPort.Close()
                                            End Sub)
                 End Sub)
    End Sub


    Private Sub DataReceivedHandler(sender As Object, e As SerialDataReceivedEventArgs)
        Dim count As Integer = serialPort.BytesToRead
        Dim buffer(count - 1) As Byte
        serialPort.Read(buffer, 0, count)

        ' Affiche les octets reçus en hexadécimal
        Dim hexStr As String = BitConverter.ToString(buffer).Replace("-", " ")
        Console.WriteLine("[" & TimeString & "]" & "Reçu : " & hexStr)
    End Sub

    Private Sub UpdateStatus(state As NLRStatus)
        Console.WriteLine("Change state : " & state.ToString())
        Dim message As String = "Error"
        Dim color As Brush = Nothing

        ' Créez le Brush et le message dans le Dispatcher pour garantir qu'ils sont créés sur le thread principal
        Dispatcher.Invoke(Sub()
                              If state = NLRStatus.CONNECTED Then
                                  message = "Connecté"
                                  color = New SolidColorBrush(Media.Color.FromRgb(0, 255, 0)) ' Vert
                              ElseIf state = NLRStatus.DISCONNECTED Then
                                  message = "Déconnecté"
                                  color = New SolidColorBrush(Media.Color.FromRgb(255, 182, 193)) ' Rose
                              End If
                          End Sub)

        ' Mettez à jour l'interface utilisateur
        Dispatcher.BeginInvoke(Sub()
                                   GroundStatus.Background = color
                                   GroundStatus.Text = message
                               End Sub)
    End Sub


    Private Sub Window_Closed(sender As Object, e As EventArgs)
        isPortOpening = False
        If serialPort IsNot Nothing AndAlso serialPort.IsOpen Then
            serialPort.Close()
        End If
        If openPortThread IsNot Nothing AndAlso openPortThread.IsAlive Then
            openPortThread.Join()
        End If
    End Sub

    Private Sub ComboBoxPorts_SelectionChanged(sender As Object, e As SelectionChangedEventArgs) Handles ComboBoxPorts.SelectionChanged
        Dim selectedport = ComboBoxPorts.SelectedItem
        If selectedport IsNot Nothing Then
            If Not isPortOpening Then
                isPortOpening = True
                openPortThread = New Thread(Sub() Me.TryOpenPort(selectedport))
                openPortThread.Start()
            End If
        End If
    End Sub

    Private Sub refresh_Click(sender As Object, e As RoutedEventArgs) Handles refresh.Click
        ListAvailablePorts()
    End Sub
End Class
